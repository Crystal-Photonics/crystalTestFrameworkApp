#include "devicematcher.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "Protocols/rpcprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "console.h"
#include "mainwindow.h"
#include "ui_devicematcher.h"
#include <QMessageBox>

DeviceMatcher::DeviceMatcher(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceMatcher) {
    ui->setupUi(this);
}

DeviceMatcher::~DeviceMatcher() {
    delete ui;
}

std::vector<std::pair<CommunicationDevice *, Protocol *>> test_acceptances(std::vector<ComportDescription *> candidates, TestRunner &runner) {
    std::vector<std::pair<CommunicationDevice *, Protocol *>> devices;
    for (auto device : candidates) {
        auto rpc_protocol = dynamic_cast<RPCProtocol *>(device->protocol.get());
        auto scpi_protocol = dynamic_cast<SCPIProtocol *>(device->protocol.get());
        if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
            sol::optional<std::string> message;
            try {
                sol::table table = runner.create_table();
                rpc_protocol->get_lua_device_descriptor(table);
                message = runner.call<sol::optional<std::string>>("RPC_acceptable", std::move(table));
            } catch (const sol::error &e) {
                const auto &message = QObject::tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
                Console::error(runner.console) << message;
                return {};
            }
            if (message) {
                //device incompatible, reason should be inside of message
                Console::note(runner.console) << QObject::tr("Device rejected:") << message.value();
                return {};
            } else {
                //acceptable device found
                devices.emplace_back(device->device.get(), device->protocol.get());
            }
        } else if (scpi_protocol) {
            sol::optional<std::string> message;
            try {
                sol::table table = runner.create_table();
                scpi_protocol->get_lua_device_descriptor(table);
                message = runner.call<sol::optional<std::string>>("SCPI_acceptable", std::move(table));
            } catch (const sol::error &e) {
                const auto &message = QObject::tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
                Console::error(runner.console) << message;
                return {};
            }
            if (message) {
                //device incompatible, reason should be inside of message
                Console::note(runner.console) << QObject::tr("Device rejected:") << message.value();
                return {};
            } else {
                //acceptable device found
                devices.emplace_back(device->device.get(), device->protocol.get());
            }
        } else {
            assert(!"TODO: handle non-RPC/SCPI protocol");
        }
    }
    return devices;
}

void DeviceMatcher::match_devices(DeviceWorker &device_worker, TestRunner &runner, TestDescriptionLoader &test) {
    successful_matching = true;
    bool over_defined_found = false;
    devices_to_match.clear();

    for (auto &device_requirement : test.get_device_requirements()) {
        DevicesToMatchEntry device_match_entry;
        std::vector<std::pair<CommunicationDevice *, Protocol *>> accepted_candidates;
        //TODO: do not only loop over comport_devices, but other devices as well
        {
            std::vector<ComportDescription *> candidates =
                device_worker.get_devices_with_protocol(device_requirement.protocol_name, device_requirement.device_names);

            // candidates.
            //TODO: skip over candidates that are already in use

            accepted_candidates = test_acceptances(candidates, runner);
        }
        device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::UnderDefined;
        if ((device_requirement.quantity_min <= (int)accepted_candidates.size()) && ((int)accepted_candidates.size() <= device_requirement.quantity_max)) {
            device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::FullDefined;
        } else if ((int)accepted_candidates.size() > device_requirement.quantity_max) {
            device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::OverDefined;
        } else if ((int)accepted_candidates.size() < device_requirement.quantity_min) {
            device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::UnderDefined;
        }
        device_match_entry.accepted_candidates = accepted_candidates;
        device_match_entry.device_requirement = device_requirement;
        for (auto c : device_match_entry.accepted_candidates) {
            (void)c;
            device_match_entry.selected_candidate.push_back(false);
        }
        switch (device_match_entry.match_definition) {
            case DevicesToMatchEntry::MatchDefinitionState::UnderDefined:
                //failed to find suitable device
                successful_matching = false;
                Console::error(runner.console) << QObject::tr(
                                                      "The selected test \"%1\" requires a device with protocol \"%2\", but no such device is available.")
                                                      .arg(test.get_name(), device_requirement.protocol_name);
                return;
            case DevicesToMatchEntry::MatchDefinitionState::FullDefined: {
                for (size_t i = 0; i < device_match_entry.selected_candidate.size(); i++) {
                    device_match_entry.selected_candidate[i] = true;
                }
            } break;
            case DevicesToMatchEntry::MatchDefinitionState::OverDefined: {
                successful_matching = false;
                over_defined_found = true;
            }

                // exec();
                //found multiple viable options
                //QMessageBox::critical(MainWindow::mw, "TODO", "TODO: implementation for multiple viable device options");
                // return;
                // }
        }
        devices_to_match.append(device_match_entry);
    }
    if (over_defined_found) {
        make_gui();
        exec();
    }
}

std::vector<std::pair<CommunicationDevice *, Protocol *>> DeviceMatcher::get_matched_devices() {
    std::vector<std::pair<CommunicationDevice *, Protocol *>> device_matching_result;
    for (auto d : devices_to_match) {
        if (d.match_definition == DevicesToMatchEntry::MatchDefinitionState::FullDefined) {
            device_matching_result.insert(device_matching_result.end(), d.accepted_candidates.begin(), d.accepted_candidates.end()); //
        }
    }
    return device_matching_result;
}

bool DeviceMatcher::was_successful() {
    return successful_matching;
}

void DeviceMatcher::on_DeviceMatcher_accepted() {}

void DeviceMatcher::make_gui() {
    ui->tree_required->setColumnCount(2);
    QList<QTreeWidgetItem *> items;
    int requirement_index_to_select = 0;
    for (auto d : devices_to_match) {
        QTreeWidgetItem *tv = new QTreeWidgetItem(ui->tree_required);

        tv->setText(0, d.device_requirement.device_names.join("/"));
        switch (d.match_definition) {
            case DevicesToMatchEntry::MatchDefinitionState::FullDefined: {
                tv->setText(1, "Ok");
            } break;
            case DevicesToMatchEntry::MatchDefinitionState::UnderDefined: {
                tv->setText(1, "Less");

            } break;
            case DevicesToMatchEntry::MatchDefinitionState::OverDefined: {
                tv->setText(1, "More");
                requirement_index_to_select = items.count();
            } break;
        }

        items.append(tv);
    }
    ui->tree_required->insertTopLevelItems(0, items);
}

//treeWidget.model().dataChanged.connect(handle_dataChanged)

void DeviceMatcher::load_available_devices(int required_index) {
    assert(devices_to_match.count() > required_index);
    ui->tree_available->clear();
    DevicesToMatchEntry requirement = devices_to_match[required_index];
    ui->tree_available->setColumnCount(2);
    QList<QTreeWidgetItem *> items;
    assert(requirement.accepted_candidates.size() == requirement.selected_candidate.size());
    for (size_t i = 0; i < requirement.accepted_candidates.size(); i++) {
        auto d = requirement.accepted_candidates[i];
        QTreeWidgetItem *tv = new QTreeWidgetItem(ui->tree_available);
        auto com_port = dynamic_cast<ComportCommunicationDevice *>(d.first);
        if (com_port) {
            tv->setText(0, com_port->port.portName());
        }
        if (requirement.selected_candidate[i]) {
            tv->setCheckState(0, Qt::Checked);
        } else {
            tv->setCheckState(0, Qt::Unchecked);
        }
        items.append(tv);
    }

    ui->tree_available->insertTopLevelItems(0, items);
}

void DeviceMatcher::on_tree_required_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    (void)previous;
    load_available_devices(ui->tree_required->indexOfTopLevelItem(current));
}
