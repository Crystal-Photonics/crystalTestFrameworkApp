#include "deviceworker.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "device_protocols_settings.h"

#include "Protocols/rpcprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "Protocols/sg04countprotocol.h"
#include "config.h"
#include "console.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "util.h"

#include <QSettings>
#include <QTimer>
#include <QTreeWidgetItem>
#include <functional>
#include <initializer_list>
#include <regex>

void DeviceWorker::poll_ports() {
    Utility::thread_call(this, [this] {
        for (auto &device : comport_devices) {
            if (device.device->isConnected()) {
                if (device.device->is_waiting_for_message() == false) {
                    device.device->waitReceived(CommunicationDevice::Duration{0}, 1, true);
                }
            }
        }
        QTimer::singleShot(16, this, &DeviceWorker::poll_ports);
    });
}

static bool is_valid_baudrate(QSerialPort::BaudRate baudrate) {
    switch (baudrate) {
        case QSerialPort::Baud1200:
        case QSerialPort::Baud2400:
        case QSerialPort::Baud4800:
        case QSerialPort::Baud9600:
        case QSerialPort::Baud38400:
        case QSerialPort::Baud57600:
        case QSerialPort::Baud19200:
        case QSerialPort::Baud115200:
            return true;
        case QSerialPort::UnknownBaud:
            return false;
    }
    return false;
}

void DeviceWorker::detect_devices(std::vector<ComportDescription *> comport_device_list) {
    auto device_protocol_settings_file = QSettings{}.value(Globals::device_protocols_file_settings_key, "").toString();
    if (device_protocol_settings_file.isEmpty()) {
        MainWindow::mw->show_message_box(tr("Missing File"), tr("Auto-Detecting devices requires a file that defines which protocols can use which device "
                                                                "types. Make such a file and add it via Settings->Paths"),
                                         QMessageBox::Critical);
        return;
    }
    DeviceProtocolsSettings device_protocol_settings{device_protocol_settings_file};
    scpi_meta_data.reload(QSettings{}.value(Globals::measurement_equipment_meta_data_path, "").toString());

    auto &rpc_devices = device_protocol_settings.protocols_rpc;
    auto &scpi_devices = device_protocol_settings.protocols_scpi;
    auto &sg04_count_devices = device_protocol_settings.protocols_sg04_count;
    auto check_rpc_protocols = [&rpc_devices, &device_protocol_settings_file](ComportDescription &device) {
        for (auto &rpc_device : rpc_devices) {
            if (rpc_device.match(device.info.portName()) == false) {
                continue;
            }

            const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(rpc_device.baud);
            if (is_valid_baudrate(baudrate) == false) {
                MainWindow::mw->show_message_box(
                    tr("Input Error"),
                    tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file),
                    QMessageBox::Critical);
                continue;
            }
            if (device.device->connect(device.info, baudrate) == false) {
                Console::warning() << tr("Failed opening") << device.device->getTarget();
                return;
            }
            auto protocol = std::make_unique<RPCProtocol>(*device.device, rpc_device);
            if (protocol->is_correct_protocol()) {
                MainWindow::mw->execute_in_gui_thread([ protocol = protocol.get(), ui_entry = device.ui_entry ] { protocol->set_ui_description(ui_entry); });
                device.protocol = std::move(protocol);
            } else {
                device.device->close();
            }
        }
    };
    auto check_scpi_protocols = [&scpi_devices, &device_protocol_settings_file, &scpi_meta_data = scpi_meta_data ](ComportDescription & device) {
        for (auto &scpi_device : scpi_devices) {
            if (scpi_device.match(device.info.portName()) == false) {
                continue;
            }

            const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(scpi_device.baud);
            if (is_valid_baudrate(baudrate) == false) {
                MainWindow::mw->show_message_box(
                    tr("Input Error"),
                    tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file),
                    QMessageBox::Critical);
                continue;
            }
            if (device.device->connect(device.info, baudrate) == false) {
                Console::warning() << tr("Failed opening") << device.device->getTarget();
                return;
            }
            auto protocol = std::make_unique<SCPIProtocol>(*device.device, scpi_device);
            if (protocol) {
                if (protocol->is_correct_protocol()) {
                    protocol->set_scpi_meta_data(
                        scpi_meta_data.query(QString().fromStdString(protocol->get_serial_number()), QString().fromStdString(protocol->get_name())));
                    MainWindow::mw->execute_in_gui_thread(
                        [ protocol = protocol.get(), ui_entry = device.ui_entry ] { protocol->set_ui_description(ui_entry); });
                    device.protocol = std::move(protocol);

                } else {
                    device.device->close();
                }
            }
        }
    };
    auto check_sg04_count_protocols = [&sg04_count_devices, &device_protocol_settings_file](ComportDescription &device) {
        for (auto &sg04_count_device : sg04_count_devices) {
            if (sg04_count_device.match(device.info.portName()) == false) {
                continue;
            }

            const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(sg04_count_device.baud);
            if (is_valid_baudrate(baudrate) == false) {
                MainWindow::mw->show_message_box(
                    tr("Input Error"),
                    tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file),
                    QMessageBox::Critical);
                continue;
            }
            if (device.device->connect(device.info, baudrate) == false) {
                Console::warning() << tr("Failed opening") << device.device->getTarget();
                return;
            }
            auto protocol = std::make_unique<SG04CountProtocol>(*device.device, sg04_count_device);
            if (protocol) {
                if (protocol->is_correct_protocol()) {

                    MainWindow::mw->execute_in_gui_thread(
                        [ protocol = protocol.get(), ui_entry = device.ui_entry ] { protocol->set_ui_description(ui_entry); });
                    device.protocol = std::move(protocol);

                } else {
                    device.device->close();
                }
            }
        }
    };

    //TODO: Add non-rpc device discovery here

    for (auto &device : comport_device_list) {
        std::function<void(ComportDescription &)> protocol_functions[] = {check_rpc_protocols, check_scpi_protocols, check_sg04_count_protocols};
        for (auto &protocol_check_function : protocol_functions) {
            if (device->device->isConnected()) {
                break;
            }
            protocol_check_function(*device);
        }
        if (device->device->isConnected() == false) { //out of protocols and still not connected
            Console::note() << tr("No protocol found for %1").arg(device->device->getTarget());
        }
    }
}

DeviceWorker::~DeviceWorker() {}

void DeviceWorker::forget_device(QTreeWidgetItem *item) {
    Utility::thread_call(this, [this, item] {
        assert(currently_in_gui_thread() == false);
        for (auto device_it = std::begin(comport_devices); device_it != std::end(comport_devices); ++device_it) {
            if (device_it->ui_entry == item) {
                device_it = comport_devices.erase(device_it);
                break;
            }
        }
    });
}

void DeviceWorker::update_devices() {
    Utility::thread_call(this, [this] {
        assert(currently_in_gui_thread() == false);
        auto portlist = QSerialPortInfo::availablePorts();
        for (auto &port : portlist) {
            auto pos = std::lower_bound(std::begin(comport_devices), std::end(comport_devices), port.systemLocation(),
                                        [](const ComportDescription &lhs, const QString &rhs) { return lhs.device->getTarget() < rhs; });
            if (pos != std::end(comport_devices) && pos->device->getTarget() == port.systemLocation()) {
                continue;
            }
            auto item = std::make_unique<QTreeWidgetItem>(QStringList{} << port.portName() + " " + port.description());

            auto &device =
                *comport_devices.insert(pos, {std::make_unique<ComportCommunicationDevice>(port.systemLocation()), port, item.get(), nullptr})->device;
            MainWindow::mw->add_device_item(item.release(), port.portName() + " " + port.description(), &device);
        }
    });
}

void DeviceWorker::detect_devices() {
    Utility::thread_call(this, [this] {
        assert(currently_in_gui_thread() == false);
        std::vector<ComportDescription *> descriptions;
        descriptions.reserve(comport_devices.size());
        for (auto &comport : comport_devices) {
            descriptions.push_back(&comport);
        }
        detect_devices(descriptions);
    });
}

void DeviceWorker::detect_device(QTreeWidgetItem *item) {
    Utility::thread_call(this, [this, item] {
        assert(currently_in_gui_thread() == false);
        for (auto &comport : comport_devices) {
            if (comport.ui_entry == item) {
                detect_devices({&comport});
                break;
            }
        }
    });
}

void DeviceWorker::connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport) {
    Utility::thread_call(this, [this, console, comport] {
        assert(currently_in_gui_thread() == false);
        struct Data {
            void (CommunicationDevice::*signal)(const QByteArray &);
            QColor color;
            bool fat;
        };
        Data data[] = {{&CommunicationDevice::received, Qt::darkGreen, false},
                       {&CommunicationDevice::decoded_received, Qt::darkGreen, true},
                       {&CommunicationDevice::message, Qt::black, false},
                       {&CommunicationDevice::sent, Qt::darkRed, false},
                       {&CommunicationDevice::decoded_sent, Qt::darkRed, true}};

        static const QString normal_html = R"(<font color="#%1"><plaintext>%2</plaintext></font>)";
        static const QString fat_html = R"(<font color="#%1"><b><plaintext>%2</plaintext></b></font>)";

        for (auto &d : data) {
            connect(comport, d.signal, [ console = console, color = d.color, fat = d.fat ](const QByteArray &data) {
                MainWindow::mw->append_html_to_console((fat ? fat_html : normal_html)
                                                           .arg(QString::number(color.rgb(), 16), Console::use_human_readable_encoding ?
                                                                                                      Utility::to_human_readable_binary_data(data) :
                                                                                                      Utility::to_C_hex_encoding(data)),
                                                       console);
            });
        }
    });
}

std::vector<ComportDescription *> DeviceWorker::get_devices_with_protocol(const QString &protocol, const QStringList device_names) {
    return Utility::promised_thread_call(this, [this, protocol, device_names]() mutable {
        assert(currently_in_gui_thread() == false);
        std::vector<ComportDescription *> candidates;
        for (auto &device : comport_devices) { //TODO: do not only loop over comport_devices, but other devices as well
            if (device.protocol == nullptr) {
                continue;
            }
            auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol.get());
            auto scpi_protocol = dynamic_cast<SCPIProtocol *>(device.protocol.get());

            bool device_name_match = false;
            QString device_name;
            if (scpi_protocol) {
                device_name = QString().fromStdString(scpi_protocol->get_name());
            } else if (rpc_protocol) {
                device_name = QString().fromStdString(rpc_protocol->get_name());
            }
            if (device_names.indexOf(device_name) > -1) {
                device_name_match = true;
            }
            if (device_names.count() == 0) {
                device_name_match = true;
            }
            for (auto s : device_names) {
                if (s == "*") {
                    device_name_match = true;
                    break;
                }
                if (s == "") {
                    device_name_match = true;
                    break;
                }
            }
            if ((device.protocol->type == protocol) && (device_name_match)) {
                candidates.push_back(&device);
            }
        }
        return candidates;
    });
}

void DeviceWorker::set_currently_running_test(CommunicationDevice *com_device, const QString &test_name) {
    for (auto &device : comport_devices) {
        if (device.device.get() == com_device) {
            device.is_in_use = test_name.size();
            MainWindow::mw->execute_in_gui_thread([ item = device.ui_entry, test_name ] { item->setText(3, test_name); });
            break;
        }
    }
}

QStringList DeviceWorker::get_string_list(ScriptEngine &script, const QString &name) {
    return Utility::promised_thread_call(this, [&script, &name] { return script.get_string_list(name); });
}
