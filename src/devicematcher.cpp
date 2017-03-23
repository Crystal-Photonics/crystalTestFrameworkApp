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
        if (device->is_in_use) {
            continue;
        }

        auto rpc_protocol = dynamic_cast<RPCProtocol *>(device->protocol.get());
        auto scpi_protocol = dynamic_cast<SCPIProtocol *>(device->protocol.get());
        if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
            sol::optional<std::string> message;
            try {
                sol::table table = runner.create_table();
                rpc_protocol->get_lua_device_descriptor(table);
                message = runner.call<sol::optional<std::string>>("RPC_acceptable", std::move(table));
            } catch (const sol::error &e) {
                const auto &message = QObject::tr("Failed to call function RPC_acceptable for device %1 \nError message: %2")
                                          .arg(QString::fromStdString(rpc_protocol->get_name()))
                                          .arg(e.what());
                Console::error(runner.console) << message;
            }
            if (message) {
                //device incompatible, reason should be inside of message
                Console::note(runner.console) << QObject::tr("Device %1 rejected:").arg(QString::fromStdString(rpc_protocol->get_name())) << message.value();
            } else {
                //acceptable device found
                devices.emplace_back(device->device.get(), device->protocol.get());
            }
        } else if (scpi_protocol) {
            if (scpi_protocol->get_approved_state() != SCPIApprovedState::Approved) {
                const auto &message = QObject::tr("SCPI device %1 with serial number =\"%2\" is lacking approval.\nIts approval state: %3")
                                          .arg(QString::fromStdString(scpi_protocol->get_name()))
                                          .arg(QString::fromStdString(scpi_protocol->get_serial_number()))
                                          .arg(scpi_protocol->get_approved_state_str());
                Console::note(runner.console) << message;
                continue;
            }
            sol::optional<std::string> message;
            try {
                sol::table table = runner.create_table();
                scpi_protocol->get_lua_device_descriptor(table);
                message = runner.call<sol::optional<std::string>>("SCPI_acceptable", std::move(table));
            } catch (const sol::error &e) {
                const auto &message = QObject::tr("Failed to call function SCPI_acceptable for device %1 with serial number =\"%2 \"  \nError message: %3")
                                          .arg(QString::fromStdString(scpi_protocol->get_name()))
                                          .arg(QString::fromStdString(scpi_protocol->get_serial_number()))
                                          .arg(e.what());

                Console::error(runner.console) << message;
                continue;
            }
            if (message) {
                //device incompatible, reason should be inside of message
                Console::note(runner.console) << QObject::tr("Device %1 \"%2\" rejected:")
                                                     .arg(QString::fromStdString(scpi_protocol->get_name()))
                                                     .arg(QString::fromStdString(scpi_protocol->get_serial_number()))
                                              << message.value();

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
            case DevicesToMatchEntry::MatchDefinitionState::UnderDefined: {
                //failed to find suitable device
                successful_matching = false;
                Console::error(runner.console)
                    << QObject::tr(
                           "The selected test \"%1\" requires %2 device(s) with protocol \"%3\", and the name \"%4\" but only %5 device(s) are available.")
                           .arg(test.get_name())
                           .arg(device_requirement.quantity_min)
                           .arg(device_requirement.protocol_name)
                           .arg(device_requirement.device_names.join("/"))
                           .arg((int)accepted_candidates.size());
            } break;
            case DevicesToMatchEntry::MatchDefinitionState::FullDefined: {
                for (size_t i = 0; i < device_match_entry.selected_candidate.size(); i++) {
                    device_match_entry.selected_candidate[i] = true;
                }
            } break;
            case DevicesToMatchEntry::MatchDefinitionState::OverDefined: {
                successful_matching = false;
                over_defined_found = true;
                for (size_t i = 0; i < device_match_entry.selected_candidate.size(); i++) {
                    device_match_entry.selected_candidate[i] = true;
                }
            }
        }
        devices_to_match.append(device_match_entry);
    }
    if (over_defined_found) {
        make_treeview();
        exec();
    }
}

std::vector<std::pair<CommunicationDevice *, Protocol *>> DeviceMatcher::get_matched_devices() {
    std::vector<std::pair<CommunicationDevice *, Protocol *>> device_matching_result;
    for (auto d : devices_to_match) {
        if (d.match_definition == DevicesToMatchEntry::MatchDefinitionState::FullDefined) {
            for (unsigned int i = 0; i < d.accepted_candidates.size(); i++) {
                if (d.selected_candidate[i]) {
                    device_matching_result.push_back(d.accepted_candidates[i]);
                }
            }
        }
    }
    return device_matching_result;
}

bool DeviceMatcher::was_successful() {
    return successful_matching;
}

void DeviceMatcher::on_DeviceMatcher_accepted() {}

void DeviceMatcher::calc_gui_match_definition() {
    int i = 0;
    bool evertything_ok = true;
    for (auto d : devices_to_match) {
        auto item = ui->tree_required->topLevelItem(i);

        switch (d.match_definition) {
            case DevicesToMatchEntry::MatchDefinitionState::FullDefined: {
                item->setText(1, "Ok");
            } break;
            case DevicesToMatchEntry::MatchDefinitionState::UnderDefined: {
                item->setText(1, "Less");
                evertything_ok = false;

            } break;
            case DevicesToMatchEntry::MatchDefinitionState::OverDefined: {
                item->setText(1, "More");
                evertything_ok = false;
            } break;
        }
        i++;
    }
    ui->btn_ok->setEnabled(evertything_ok);
}

void DeviceMatcher::make_treeview() {
    ui->tree_required->setColumnCount(2);
    QList<QTreeWidgetItem *> items;
    for (auto d : devices_to_match) {
        QTreeWidgetItem *tv = new QTreeWidgetItem(ui->tree_required);
        tv->setText(0, d.device_requirement.device_names.join("/"));

        items.append(tv);
    }
    ui->tree_required->insertTopLevelItems(0, items);
    calc_gui_match_definition();
}

void DeviceMatcher::load_available_devices(int required_index) {
    assert(devices_to_match.count() > required_index);
    ui->tree_available->clear();
    DevicesToMatchEntry &requirement = devices_to_match[required_index];
    ui->tree_available->setColumnCount(3);
    QList<QTreeWidgetItem *> items;
    assert(requirement.accepted_candidates.size() == requirement.selected_candidate.size());

    for (size_t i = 0; i < requirement.accepted_candidates.size(); i++) {
        auto d = requirement.accepted_candidates[i];
        QTreeWidgetItem *tv = new QTreeWidgetItem(ui->tree_available);
        auto com_port = dynamic_cast<ComportCommunicationDevice *>(d.first);
        auto scpi_protocol = dynamic_cast<SCPIProtocol *>(d.second);
        auto rpc_protocol = dynamic_cast<RPCProtocol *>(d.second);
        if (com_port) {
            tv->setText(0, com_port->port.portName());
            if (scpi_protocol) {
                QTreeWidgetItem *tv_child = new QTreeWidgetItem(tv);
                tv_child->setText(0, scpi_protocol->get_device_summary());
                tv->setText(1, QString::fromStdString(scpi_protocol->get_serial_number()));
                tv->setText(2, scpi_protocol->get_approved_state_str());
            } else if (rpc_protocol) {
                //QTreeWidgetItem *tv_child = new QTreeWidgetItem(tv);
                //tv_child->setText(0, rpc_protocol->get_device_summary());
            }
        }

        if (requirement.selected_candidate[i]) {
            tv->setCheckState(0, Qt::Checked);
        } else {
            tv->setCheckState(0, Qt::Unchecked);
        }

        items.append(tv);
    }

    ui->tree_available->insertTopLevelItems(0, items);
    selected_requirement = &requirement;
}

void DeviceMatcher::calc_requirement_definitions() {
    for (auto &device_match_entry : devices_to_match) {
        int selected_devices = 0;
        for (auto sc : device_match_entry.selected_candidate) {
            if (sc) {
                selected_devices++;
            }
        }
        device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::UnderDefined;
        int quantity_min = device_match_entry.device_requirement.quantity_min;
        int quantity_max = device_match_entry.device_requirement.quantity_max;
        if ((quantity_min <= selected_devices) && (selected_devices <= quantity_max)) {
            device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::FullDefined;
        } else if (selected_devices > quantity_max) {
            device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::OverDefined;
        } else if (selected_devices < quantity_min) {
            device_match_entry.match_definition = DevicesToMatchEntry::MatchDefinitionState::UnderDefined;
        }
    }
    calc_gui_match_definition();
}

void DeviceMatcher::on_tree_required_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    (void)previous;
    selected_requirement = nullptr;
    load_available_devices(ui->tree_required->indexOfTopLevelItem(current));
}

void DeviceMatcher::on_tree_available_itemChanged(QTreeWidgetItem *item, int column) {
    (void)column;
    if (selected_requirement) {
        int row = ui->tree_available->indexOfTopLevelItem(item);
        if ((row > -1) && (row < selected_requirement->selected_candidate.size())) {
            if (item->checkState(0) == Qt::Checked) {
                selected_requirement->selected_candidate[row] = true;
                if (selected_requirement->device_requirement.quantity_max == 1) {
                    for (unsigned int i = 0; i < selected_requirement->selected_candidate.size(); i++) {
                        if (i == row) {
                            continue;
                        }

                        auto item_to_uncheck = ui->tree_available->topLevelItem(i);
                        item_to_uncheck->setCheckState(0, Qt::Unchecked);
                    }
                }
            } else if (item->checkState(0) == Qt::Unchecked) {
                selected_requirement->selected_candidate[row] = false;
            }
        }
        calc_requirement_definitions();
    }
}

void DeviceMatcher::on_btn_cancel_clicked() {
    close();
}

void DeviceMatcher::on_btn_ok_clicked() {
    successful_matching = true;
    close();
}
