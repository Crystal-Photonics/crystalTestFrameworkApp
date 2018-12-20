#include "devicematcher.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "Protocols/manualprotocol.h"
#include "Protocols/rpcprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "Protocols/sg04countprotocol.h"
#include "console.h"
#include "mainwindow.h"
#include "ui_devicematcher.h"
#include <QMessageBox>
#include <QTimer>

DeviceMatcher::DeviceMatcher(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceMatcher) {
    ui->setupUi(this);
    QTimer::singleShot(500, this, &DeviceMatcher::poll_sg04_counts);
}

DeviceMatcher::~DeviceMatcher() {
    delete ui;
}

std::vector<std::pair<CommunicationDevice *, Protocol *>> test_acceptances(std::vector<PortDescription *> candidates, TestRunner &runner) {
    std::vector<std::pair<CommunicationDevice *, Protocol *>> devices;
    for (auto device : candidates) {
        if (device->get_is_in_use()) {
            continue;
        }

        auto rpc_protocol = dynamic_cast<RPCProtocol *>(device->protocol.get());
        auto scpi_protocol = dynamic_cast<SCPIProtocol *>(device->protocol.get());
        auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(device->protocol.get());
        auto manual_protocol = dynamic_cast<ManualProtocol *>(device->protocol.get());
        if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
            sol::optional<std::string> message;
            try {
                sol::table table = runner.create_table();
                rpc_protocol->get_lua_device_descriptor(table);
                message = runner.call<sol::optional<std::string>>("RPC_acceptable", std::move(table)); //TODO: If fails, the program should not crash
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
            if (scpi_protocol->get_approved_state() != DeviceMetaDataApprovedState::Approved) {
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
                message = runner.call<sol::optional<std::string>>("SCPI_acceptable", std::move(table)); //TODO: If fails, the program should not crash
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

        } else if (sg04_count_protocol) {
            {
                //acceptable device found
                devices.emplace_back(device->device.get(), device->protocol.get());
            }
        } else if (manual_protocol) {
            {
                if (manual_protocol->get_approved_state() != DeviceMetaDataApprovedState::Approved) {
                    const auto &message = QObject::tr("Manual device %1 with serial number =\"%2\" is lacking approval.\nIts approval state: %3")
                                              .arg(QString::fromStdString(manual_protocol->get_name()))
                                              .arg(QString::fromStdString(manual_protocol->get_serial_number()))
                                              .arg(manual_protocol->get_approved_state_str());
                    Console::note(runner.console) << message;
                    continue;
                }
                //acceptable device found
                devices.emplace_back(device->device.get(), device->protocol.get());
            }

        } else {
            assert(!"TODO: handle non-RPC/SCPI/SG04 protocol");
        }
    }
    return devices;
}

bool DeviceMatcher::is_match_possible(DeviceWorker &device_worker, TestDescriptionLoader &test) {
    for (auto &device_requirement : test.get_device_requirements()) {

            std::vector<PortDescription *> candidates =
                device_worker.get_devices_with_protocol(device_requirement.protocol_name, device_requirement.device_names);

        if ((int)candidates.size() < device_requirement.quantity_min) {
            return false;
        }
    }

    return true;
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
            std::vector<PortDescription *> candidates =
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
        for (auto ce : accepted_candidates) {
            CandidateEntry candidate_entry{};
            candidate_entry.communication_device = ce.first;
            candidate_entry.protocol = ce.second;
            candidate_entry.selected = false;
            device_match_entry.accepted_candidates.push_back(candidate_entry);
        }
        device_match_entry.device_requirement = device_requirement;

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
                for (auto &ce : device_match_entry.accepted_candidates) {
                    ce.selected = true;
                }
            } break;
            case DevicesToMatchEntry::MatchDefinitionState::OverDefined: {
                successful_matching = false;
                over_defined_found = true;
                for (auto &ce : device_match_entry.accepted_candidates) {
                    ce.selected = true;
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

std::vector<MatchedDevice> DeviceMatcher::get_matched_devices() {
    std::vector<MatchedDevice> device_matching_result;
    for (auto d : devices_to_match) {
        if (d.match_definition == DevicesToMatchEntry::MatchDefinitionState::FullDefined) {
            //qDebug() << "get_matched_devices" << d.device_requirement.alias;
            for (auto &ac : d.accepted_candidates) {
                if (ac.selected) {
                    MatchedDevice p{ac.communication_device, ac.protocol, d.device_requirement.alias};
                    device_matching_result.push_back(p);
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
    int select_index = 0;
    bool first_match_error_found = false;
    QList<QTreeWidgetItem *> items;
    for (auto d : devices_to_match) {
        QTreeWidgetItem *tv = new QTreeWidgetItem(ui->tree_required);
        tv->setText(0, d.device_requirement.device_names.join("/"));
        if (d.device_requirement.device_names.join("").count() == 0) {
            tv->setText(0, d.device_requirement.protocol_name);
        } else if (d.device_requirement.device_names.join("") == "*") {
            tv->setText(0, d.device_requirement.protocol_name);
        }
        if ((d.match_definition != DevicesToMatchEntry::MatchDefinitionState::FullDefined) && (!first_match_error_found)) {
            first_match_error_found = true;
            select_index = items.count();
        }
        items.append(tv);
    }
    ui->tree_required->insertTopLevelItems(0, items);
    calc_gui_match_definition();
    align_columns();
    ui->tree_required->setCurrentItem(ui->tree_required->topLevelItem(select_index));
}

void DeviceMatcher::load_available_devices(int required_index) {
    assert(devices_to_match.count() > required_index);
    ui->tree_available->clear();
    DevicesToMatchEntry &requirement = devices_to_match[required_index];
    ui->tree_available->setColumnCount(3);
    QList<QTreeWidgetItem *> items;

    QStringList table_captions;

    for (auto &d : requirement.accepted_candidates) {
        QTreeWidgetItem *tv = new QTreeWidgetItem(ui->tree_available);
        //auto com_port = dynamic_cast<ComportCommunicationDevice *>(d.communication_device);
        auto scpi_protocol = dynamic_cast<SCPIProtocol *>(d.protocol);
        auto rpc_protocol = dynamic_cast<RPCProtocol *>(d.protocol);
        auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(d.protocol);
        auto manual_protocol = dynamic_cast<ManualProtocol *>(d.protocol);

        tv->setText(0, d.communication_device->get_port_info()[HOST_NAME_TAG].toString());
        if (scpi_protocol) {
            QTreeWidgetItem *tv_child = new QTreeWidgetItem(tv);
            tv_child->setText(0, scpi_protocol->get_device_summary());
            tv->setText(1, QString::fromStdString(scpi_protocol->get_serial_number()));
            tv->setText(2, scpi_protocol->get_approved_state_str());
            table_captions = QStringList{"Port", "Serialnumber", "Calibration"};
        } else if (rpc_protocol) {
            QTreeWidgetItem *tv_child = new QTreeWidgetItem(tv);
            tv_child->setText(0, rpc_protocol->get_device_summary());
            table_captions = QStringList{"Port", "Serialnumber", "Calibration"};
            //TODO shall we put more information here?
        } else if (sg04_count_protocol) {
            tv->setText(1, "SG04");
            table_captions = QStringList{"Port", "", "Actual count-rate"};
        } else if (manual_protocol) {
            tv->setText(0, d.communication_device->get_port_info()[DEVICE_MANUAL_NAME_TAG].toString() + "(" +
                               QString::fromStdString(manual_protocol->get_serial_number()) + ")");
            tv->setText(1, QString::fromStdString(manual_protocol->get_notes()));
            tv->setText(2, manual_protocol->get_approved_state_str());
            table_captions = QStringList{"Name(sn)", "Notes", "Calibration"};
        }

        if (d.selected) {
            tv->setCheckState(0, Qt::Checked);
        } else {
            tv->setCheckState(0, Qt::Unchecked);
        }

        items.append(tv);
    }

    ui->tree_available->setHeaderLabels(table_captions);
    ui->tree_available->insertTopLevelItems(0, items);
    selected_requirement = &requirement;
    align_columns();
}

void DeviceMatcher::calc_requirement_definitions() {
    for (auto &device_match_entry : devices_to_match) {
        int selected_devices = 0;
        for (auto sc : device_match_entry.accepted_candidates) {
            if (sc.selected) {
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

void DeviceMatcher::align_columns() {
    ui->tree_required->expandAll();
    ui->tree_available->expandAll();
    for (int i = 0; i < ui->tree_available->columnCount(); i++) {
        ui->tree_available->resizeColumnToContents(i);
    }
    for (int i = 0; i < ui->tree_required->columnCount(); i++) {
        ui->tree_required->resizeColumnToContents(i);
    }
}

void DeviceMatcher::on_tree_available_itemChanged(QTreeWidgetItem *item, int column) {
    (void)column;
    if (selected_requirement) {
        int row = ui->tree_available->indexOfTopLevelItem(item);
        if ((row > -1) && (row < (int)selected_requirement->accepted_candidates.size())) {
            if (item->checkState(0) == Qt::Checked) {
                selected_requirement->accepted_candidates[row].selected = true;
                if (selected_requirement->device_requirement.quantity_max == 1) {
                    for (int i = 0; i < (int)selected_requirement->accepted_candidates.size(); i++) {
                        if (i == row) {
                            continue;
                        }
                        auto item_to_uncheck = ui->tree_available->topLevelItem(i);
                        item_to_uncheck->setCheckState(0, Qt::Unchecked);
                    }
                }
            } else if (item->checkState(0) == Qt::Unchecked) {
                selected_requirement->accepted_candidates[row].selected = false;
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

void DeviceMatcher::on_btn_check_all_clicked() {
    for (unsigned int i = 0; i < selected_requirement->accepted_candidates.size(); i++) {
        auto item_to_uncheck = ui->tree_available->topLevelItem(i);
        item_to_uncheck->setCheckState(0, Qt::Checked);
    }
}

void DeviceMatcher::on_btn_uncheck_all_clicked() {
    for (unsigned int i = 0; i < selected_requirement->accepted_candidates.size(); i++) {
        auto item_to_uncheck = ui->tree_available->topLevelItem(i);
        item_to_uncheck->setCheckState(0, Qt::Unchecked);
    }
}

void PortDescription::set_is_in_use(bool in_use) {
    device.get()->set_is_in_use(in_use);
}

bool PortDescription::get_is_in_use() {
    return device.get()->get_is_in_use();
}

void DeviceMatcher::poll_sg04_counts() {
    const QString sg04_prot_string = "SG04Count";
    int index = 0;
    if (selected_requirement) {
        if (selected_requirement->device_requirement.protocol_name == sg04_prot_string) {
            for (auto &candidate_entry : selected_requirement->accepted_candidates) {
                auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(candidate_entry.protocol);
                if (sg04_count_protocol) {
                    unsigned int cps = sg04_count_protocol->get_actual_count_rate_cps();
                    ui->tree_available->topLevelItem(index)->setText(2, "cps: " + QString::number(cps));
                }
                index++;
            }
        }
    }

    QTimer::singleShot(500, this, &DeviceMatcher::poll_sg04_counts);
}

QString ComDeviceTypeToString(CommunicationDeviceType t) {
    switch (t) {
        case CommunicationDeviceType::COM:
            return "COM";
        case CommunicationDeviceType::TMC:
            return "TMC";
        case CommunicationDeviceType::Manual:
            return "Manual";
        case CommunicationDeviceType::TCP:
            return "TCP";
        case CommunicationDeviceType::UDP:
            return "UDP";
        case CommunicationDeviceType::IP:
            return "IP";
    }
    return "Unknown";
}
