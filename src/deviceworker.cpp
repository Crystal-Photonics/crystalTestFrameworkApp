#include "deviceworker.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "CommunicationDevices/dummycommunicationdevice.h"
#include "CommunicationDevices/usbtmccommunicationdevice.h"
#include "Protocols/manualprotocol.h"
#include "Protocols/rpcprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "Protocols/sg04countprotocol.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "console.h"
#include "device_protocols_settings.h"
#include "util.h"

#include <QChar>
#include <QDebug>
#include <QPlainTextEdit>
#include <QSettings>
#include <QString>
#include <QTreeWidgetItem>
#include <functional>
#include <initializer_list>

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

#if 0
const static QVector<QChar> spinner_characters = {'-', '\\', '|', '/'};

static QString progress_spinner(QString in) {
	if (in.size() == 0) {
		return in;
	}
	int index = spinner_characters.indexOf(in[in.size() - 1]);
	if (index > -1) {
		index++;
		if (index >= spinner_characters.size()) {
			index = 0;
		}
		in[in.size() - 1] = spinner_characters[index];
	} else {
		in = in + " " + spinner_characters[0];
	}
	return in;
}

static QString remove_spinner(QString in) {
	if (in.size() == 0) {
		return in;
	}
	int index = spinner_characters.indexOf(in[in.size() - 1]);
	if (index > -1) {
		auto ch = spinner_characters[index];
		if (in.endsWith(QString(" ") + ch)) {
			in.remove(in.size() - 2, 2);
		}
	}
	return in;
}
#endif

static void detect_device(QObject *device_worker, PortDescription &device, const DeviceProtocolsSettings &device_protocol_settings,
                          const QString &device_protocol_settings_file, const DeviceMetaData &device_meta_data) {
    auto &rpc_devices = device_protocol_settings.protocols_rpc;
    auto check_rpc_protocols = [device_worker, &rpc_devices, &device_protocol_settings_file](PortDescription &device) {
        for (auto &rpc_device : rpc_devices) {
            if (rpc_device.match(device.port_info[HOST_NAME_TAG].toString()) == false) {
                continue;
            }

            const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(rpc_device.baud);
            if (is_valid_baudrate(baudrate) == false) {
                MainWindow::mw->show_message_box(
                    QObject::tr("Input Error"),
                    QObject::tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file),
                    QMessageBox::Critical);
                continue;
            }
            device.port_info.insert(TYPE_NAME_TAG, "rpc");
            device.port_info.insert(BAUD_RATE_TAG, rpc_device.baud); //TODO: crash bei einem ttyUSB0, kubuntu16.04 ??
            if (device.device->connect(device.port_info) == false) {
                auto display_string = device.device->get_identifier_display_string();
                Utility::thread_call(MainWindow::mw, [display_string, ui_entry = device.ui_entry] {
                    ui_entry->setForeground(0, Qt::gray);
                    Console_handle::warning() << QObject::tr("Failed opening") << display_string;
                });
                return;
            }
            std::unique_ptr<RPCProtocol> protocol =
                Utility::promised_thread_call(device_worker, [&] { return std::make_unique<RPCProtocol>(*device.device, rpc_device); });

            if (protocol->is_correct_protocol()) {
                MainWindow::mw->execute_in_gui_thread([protocol = protocol.get(), ui_entry = device.ui_entry] { //
                    if (MainWindow::mw->device_item_exists(ui_entry)) {
                        protocol->set_ui_description(ui_entry);
                    }
                });
                device.protocol = std::move(protocol);
            } else {
                device.device->close();
            }
        }
    };
    auto &scpi_devices = device_protocol_settings.protocols_scpi;
    auto check_scpi_protocols = [&scpi_devices, &device_protocol_settings_file, &device_meta_data = device_meta_data](PortDescription &device) {
        for (auto &scpi_device : scpi_devices) {
            if (scpi_device.match(device.port_info[HOST_NAME_TAG].toString()) == false) {
                continue;
            }

            if (scpi_device.type == DeviceProtocolSetting::comport) {
                const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(scpi_device.baud);
                if (is_valid_baudrate(baudrate) == false) {
                    MainWindow::mw->show_message_box(
                        QObject::tr("Input Error"),
                        QObject::tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file),
                        QMessageBox::Critical);
                    continue;
                }
                device.port_info.insert(TYPE_NAME_TAG, "scpi");
                device.port_info.insert(BAUD_RATE_TAG, baudrate);
            }
            if (device.device->connect(device.port_info) == false) {
                auto display_string = device.device->get_identifier_display_string();
                MainWindow::mw->execute_in_gui_thread([display_string] { //
                    Console_handle::warning() << QObject::tr("Failed opening") << display_string;
                });
                return;
            }
            auto protocol = std::make_unique<SCPIProtocol>(*device.device, scpi_device);
            if (protocol) {
                if (protocol->is_correct_protocol()) {
                    protocol->set_scpi_meta_data(
                        device_meta_data.query(QString::fromStdString(protocol->get_serial_number()), QString::fromStdString(protocol->get_name())));
                    MainWindow::mw->execute_in_gui_thread([protocol = protocol.get(), ui_entry = device.ui_entry] { //
                        if (MainWindow::mw->device_item_exists(ui_entry)) {
                            protocol->set_ui_description(ui_entry);
                        }

                    });
                    device.protocol = std::move(protocol);

                } else {
                    device.device->close();
                }
            }
        }
    };
    auto &sg04_count_devices = device_protocol_settings.protocols_sg04_count;
    auto check_sg04_count_protocols = [&sg04_count_devices, &device_protocol_settings_file](PortDescription &device) {
        for (auto &sg04_count_device : sg04_count_devices) {
            if (sg04_count_device.match(device.port_info[HOST_NAME_TAG].toString()) == false) {
                continue;
            }

            const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(sg04_count_device.baud);

            if (is_valid_baudrate(baudrate) == false) {
                MainWindow::mw->show_message_box(
                    QObject::tr("Input Error"),
                    QObject::tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file),
                    QMessageBox::Critical);
                continue;
            }
            device.port_info.insert(BAUD_RATE_TAG, baudrate);
            device.port_info.insert(TYPE_NAME_TAG, "sg04");
            if (device.device->connect(device.port_info) == false) {
                auto display_string = device.device->get_identifier_display_string();
                Utility::thread_call(MainWindow::mw,
                                     [display_string] { //
                                         Console_handle::warning() << QObject::tr("Failed opening") << display_string;
                                     });
                return;
            }
            auto protocol = std::make_unique<SG04CountProtocol>(*device.device, sg04_count_device);
            if (protocol) {
                if (protocol->is_correct_protocol()) {
                    MainWindow::mw->execute_in_gui_thread([protocol = protocol.get(), ui_entry = device.ui_entry] { //
                        if (MainWindow::mw->device_item_exists(ui_entry)) {
                            protocol->set_ui_description(ui_entry);
                        }
                    });
                    device.protocol = std::move(protocol);

                } else {
                    device.device->close();
                }
            }
        }
    };
    auto check_manual_protocols = [&device_meta_data = device_meta_data](PortDescription &device) {
        if (device.communication_type != CommunicationDeviceType::Manual) {
            return;
        }
        if (device.device->connect(device.port_info) == false) {
            auto display_string = device.device->get_identifier_display_string();
            Utility::thread_call(MainWindow::mw,
                                 [display_string] { //
                                     Console_handle::warning() << QObject::tr("Failed opening") << display_string;
                                 });
            return;
        }
        auto protocol = std::make_unique<ManualProtocol>();
        if (protocol) {
            if (protocol->is_correct_protocol()) {
                protocol->set_meta_data(
                    device_meta_data.query(device.port_info[DEVICE_MANUAL_SN_TAG].toString(), device.port_info[DEVICE_MANUAL_NAME_TAG].toString()));
                Utility::thread_call(MainWindow::mw, [protocol = protocol.get(), ui_entry = device.ui_entry] { //
                    if (MainWindow::mw->device_item_exists(ui_entry)) {
                        protocol->set_ui_description(ui_entry);
                    } //
                });
                device.protocol = std::move(protocol);

            } else {
                device.device->close();
            }
        }
    };

    auto device_name = device.port_info[HOST_NAME_TAG].toString();
    std::function<void(PortDescription &)> protocol_functions[] = {check_rpc_protocols, check_scpi_protocols, check_sg04_count_protocols,
                                                                   check_manual_protocols};
    for (auto &protocol_check_function : protocol_functions) {
        if ((device.device->isConnected()) && (device.communication_type != CommunicationDeviceType::Manual)) {
            break;
        }
        try {
            protocol_check_function(device);
        } catch (std::runtime_error &e) {
            Console_handle::error() << QObject::tr("Error happened while checking device '%1'. Message: '%2'").arg(device_name).arg(QString(e.what()));
        } catch (std::domain_error &e) {
            Console_handle::error() << QObject::tr("Error happened while checking device '%1'. Message: '%2'").arg(device_name).arg(QString(e.what()));
        }
    }
    if (device.device->isConnected() == false) { //out of protocols and still not connected
        auto display_string = device.device->get_identifier_display_string();
        Utility::thread_call(MainWindow::mw, [display_string] {
            assert(currently_in_gui_thread() == true);
            Console_handle::note() << QObject::tr("No protocol found for %1").arg(display_string);
        });
    }
}

void DeviceWorker::detect_devices(std::vector<PortDescription *> device_list) {
    assert(currently_in_devices_thread());
    const auto device_protocol_settings_file = QSettings{}.value(Globals::device_protocols_file_settings_key, "").toString();
    if (device_protocol_settings_file.isEmpty()) {
        MainWindow::mw->show_message_box(tr("Missing File"),
                                         tr("Auto-Detecting devices requires a file that defines which protocols can use which device "
                                            "types. Make such a file and add it via Settings->Paths"),
                                         QMessageBox::Critical);
        return;
    }
    const DeviceProtocolsSettings device_protocol_settings{device_protocol_settings_file};
    device_meta_data.reload(QSettings{}.value(Globals::measurement_equipment_meta_data_path_key, "").toString());

    /* We want to detect each device in its own thread to speed up detection.
     * However, if there are duplicate devices, the threads interfere with each other.
     * To fix that, we sort the devices into groups where each group uses the same physical device and give the whole group to a thread.
     */
    std::map<CommunicationDevice *, std::vector<PortDescription *>> devices_map;
    for (auto &device : device_list) {
        devices_map[device->device.get()].push_back(device);
    }

    std::vector<std::future<void>> threads;
    for (auto &com_devs : devices_map) {
        threads.push_back(
            std::async(std::launch::async, [&device_protocol_settings, &device_protocol_settings_file, this, devices = std::move(com_devs.second)] {
                for (auto &dev : devices) {
                    ::detect_device(this, *dev, device_protocol_settings, device_protocol_settings_file, device_meta_data);
                }
            }));
    }
    for (auto &thread : threads) {
        while (thread.wait_for(std::chrono::milliseconds{16}) == std::future_status::timeout) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                throw sol::error("interrupted");
            }
            QApplication::processEvents();
        }
    }
}

DeviceWorker::~DeviceWorker() {}

void DeviceWorker::refresh_devices(QTreeWidgetItem *root, bool dut_only) {
    assert(currently_in_gui_thread());
    if (not refresh_semaphore.tryAcquire()) {
        //someone else is currently refreshing devices. Good enough.
        return;
    }
    QList<QTreeWidgetItem *> device_items = MainWindow::mw->get_devices_to_forget_by_root_treewidget(root);

    for (int j = 0; j < device_items.count(); j++) {
        auto item = device_items[j];
        bool forget = true;
        bool remove_from_gui = false;
        if (is_connected_to_device(item)) {
            remove_from_gui = true;
        }
        if (is_device_in_use(item)) {
            forget = false;
        }
        if (dut_only && !is_dut_device(item)) {
            forget = false;
        }

        if (forget) {
            if (remove_from_gui) {
                MainWindow::mw->remove_device_item(item);
            }
        } else {
            device_items.removeAt(j);
            j--;
            //qDebug() << "inuse:" << item->text(0);
        }
    }

    Utility::thread_call(this, [this, device_items] {
        for (auto &item : device_items) {
            forget_device(item);
        }
        device_meta_data.reload(QSettings{}.value(Globals::measurement_equipment_meta_data_path_key, "").toString());
        update_devices();
        QApplication::processEvents();
        detect_devices();
        emit device_discovery_done();
        refresh_semaphore.release();
    });
}

bool DeviceWorker::is_dut_device_(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto device_it = std::begin(communication_devices); device_it != std::end(communication_devices); ++device_it) {
        if (device_it->ui_entry == item) {
            auto rpc_protocol = dynamic_cast<RPCProtocol *>(device_it->protocol.get());
            auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(device_it->protocol.get());
            if (rpc_protocol) {
                return true;
            } else if (sg04_count_protocol) {
                return true;
            }
            break;
        }
    }
    return false;
}

bool DeviceWorker::is_dut_device(QTreeWidgetItem *item) {
    return Utility::promised_thread_call(this, [this, item] {
        assert(currently_in_gui_thread() == false);
        return is_dut_device_(item);
    });
}

bool DeviceWorker::is_device_in_use_(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto &device : communication_devices) {
        if (device.ui_entry == item) {
            return device.device->is_in_use();
        }
    }
    // qDebug() << "is_device_in_use_:"
    //        << "not found" << item->text(0);
    return false;
}

bool DeviceWorker::is_connected_to_device_(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto &device : communication_devices) {
        if (device.ui_entry == item) {
            return true;
        }
    }
    return false;
}

bool DeviceWorker::is_connected_to_device(QTreeWidgetItem *item) {
    return Utility::promised_thread_call(this, [this, item] {
        assert(currently_in_devices_thread());
        return is_connected_to_device_(item);
    });
}

bool DeviceWorker::is_device_open(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto &device : communication_devices) {
        if (device.ui_entry == item) {
            return device.device->isConnected();
        }
    }
    qDebug() << "Warning: Asking if a device is open that was not found";
    return false;
}

bool DeviceWorker::is_device_opening(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto &device : communication_devices) {
        if (device.ui_entry == item) {
            return device.device->isConnecting();
        }
    }
    qDebug() << "Warning: Asking if a device is opening that was not found";
    return false;
}

void device_worker_set_in_use(CommunicationDevice *com_device, bool in_use) {
    com_device->in_use = in_use;
}

void DeviceWorker::close_device(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto &device : communication_devices) {
        if (device.ui_entry == item) {
            device.device->close();
            device_worker_set_in_use(device.device.get(), false);
        }
    }
}

void DeviceWorker::open_device(QTreeWidgetItem *item) {
    detect_device(item);
}

QString DeviceWorker::get_manual(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    for (auto &device : communication_devices) {
        if (device.ui_entry == item) {
            return device.protocol ? device.protocol->get_manual() : QString{};
        }
    }
    qDebug() << "Warning: Failed to find Device for item" << item;
    return {};
}

bool DeviceWorker::is_device_in_use(QTreeWidgetItem *item) {
    return Utility::promised_thread_call(this, [this, item] {
        assert(currently_in_gui_thread() == false);
        return is_device_in_use_(item);
    });
}

void DeviceWorker::forget_device(QTreeWidgetItem *item) {
    assert(currently_in_devices_thread());
    const auto device_it = std::find_if(std::begin(communication_devices), std::end(communication_devices),
                                        [item](const PortDescription &communication_device) { return communication_device.ui_entry == item; });
    if (device_it != std::end(communication_devices)) {
        communication_devices.erase(device_it);
    }
}

void DeviceWorker::update_devices() {
    assert(currently_in_devices_thread());
    auto portlist = QSerialPortInfo::availablePorts();
    for (auto &port : portlist) {
        QMap<QString, QVariant> port_info;
        port_info.insert(HOST_NAME_TAG, QString(port.portName()));

        if (contains_port(port_info)) {
            continue;
        }

        communication_devices.push_back(PortDescription{
            std::make_unique<ComportCommunicationDevice>(), port_info,
            std::make_unique<QTreeWidgetItem>(QStringList{} << port.portName() + " " + port.description()).release(), nullptr, CommunicationDeviceType::COM});

        PortDescription *port_desc = &communication_devices.back();
        CommunicationDevice *device = port_desc->device.get();
        MainWindow::mw->add_device_item(port_desc->ui_entry, port.portName() + " " + port.description(), device);
    }

    for (auto manual_device : device_meta_data.get_manual_devices()) {
        QMap<QString, QVariant> port_info;
        port_info.insert(HOST_NAME_TAG, QString("manual"));

        port_info.insert(DEVICE_MANUAL_NAME_TAG, manual_device.commondata.device_name);
        port_info.insert(DEVICE_MANUAL_SN_TAG, manual_device.detail.serial_number);
        if (contains_port(port_info)) {
            continue;
        }

        {
            bool manual_devices_existing = device_meta_data.get_manual_devices().count() > 0;
            //Utility::thread_call(MainWindow::mw, [this, manual_device, item_text, port_info, manual_devices_existing] {

            QTreeWidgetItem *manual_parent_item = MainWindow::mw->get_manual_devices_parent_item();
            if (manual_parent_item == nullptr) {
                if (manual_devices_existing) {
                    //manual_parent_item = std::make_unique<QTreeWidgetItem>(QStringList{} << "Manual Devices");
                    //MainWindow::mw->set_manual_devices_parent_item(manual_parent_item);
                    manual_parent_item = MainWindow::mw->create_manual_devices_parent_item();
                }
            }

            QTreeWidgetItem *parent_item = manual_parent_item;

            const auto item_text = QStringList{} << manual_device.commondata.device_name + " (" + manual_device.detail.serial_number + ")";
            auto ui_entry = std::make_unique<QTreeWidgetItem>(item_text);
            const auto ui_entry_p = ui_entry.release();

            communication_devices.push_back(
                PortDescription{std::make_unique<DummyCommunicationDevice>(), port_info, ui_entry_p, nullptr, CommunicationDeviceType::Manual});
            MainWindow::mw->add_device_child_item(parent_item, ui_entry_p, "test", nullptr);
        }
    }

    auto tmc_devices = LIBUSBScan::scan();

    for (auto &port : tmc_devices) {
        QMap<QString, QVariant> port_info;
        port_info.insert(HOST_NAME_TAG, QString(port));

        if (contains_port(port_info)) {
            continue;
        }

        communication_devices.push_back(PortDescription{std::make_unique<USBTMCCommunicationDevice>(), port_info,
                                                        std::make_unique<QTreeWidgetItem>(QStringList{} << port).release(), nullptr,
                                                        CommunicationDeviceType::TMC});
        PortDescription *port_desc = &communication_devices.back();
        CommunicationDevice *device = port_desc->device.get();
        MainWindow::mw->add_device_item(port_desc->ui_entry, port, device);
    }

    //});
}

void DeviceWorker::detect_devices() {
    assert(currently_in_devices_thread());
    std::vector<PortDescription *> descriptions;
    descriptions.reserve(communication_devices.size());
    for (auto &comport : communication_devices) {
        descriptions.push_back(&comport);
    }
    detect_devices(descriptions);
}

void DeviceWorker::detect_device(QTreeWidgetItem *item) {
    Utility::promised_thread_call(this, [this, item] {
        assert(currently_in_gui_thread() == false);
        for (auto &comport : communication_devices) {
            if (comport.ui_entry == item) {
                detect_devices({&comport});
                break;
            }
        }
    });
}

void DeviceWorker::connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport) {
    Utility::thread_call(this, [console, comport] {
        assert(currently_in_devices_thread());
        DummyCommunicationDevice *is_dummy = dynamic_cast<DummyCommunicationDevice *>(comport);
        if (is_dummy) {
            return;
        }
        struct DataByteArgument {
            void (CommunicationDevice::*signal)(const QByteArray &);
            QColor color;
            bool fat;
        };

        struct DataNoArguement {
            void (CommunicationDevice::*signal)();
            QColor color;
            bool fat;
            QString text;
        };
        DataByteArgument data_byte_arguement[] = {{&CommunicationDevice::received, Qt::darkGreen, false},        //
                                                  {&CommunicationDevice::decoded_received, Qt::darkGreen, true}, //
                                                  {&CommunicationDevice::message, Qt::black, false},
                                                  {&CommunicationDevice::sent, Qt::darkRed, false},
                                                  {&CommunicationDevice::decoded_sent, Qt::darkRed, true},
                                                  {&CommunicationDevice::disconnected, Qt::black, true},
                                                  {&CommunicationDevice::connected, Qt::black, true}};

        static const QString normal_html = R"(<font color="#%1"><plaintext>%2</plaintext></font>)";
        static const QString fat_html = R"(<font color="#%1"><b><plaintext>%2</plaintext></b></font>)";

        for (auto &d : data_byte_arguement) {
            connect(comport, d.signal, [signal = d.signal, console = console, color = d.color, fat = d.fat](const QByteArray &data) {
                QString display_text = data;
                bool is_human_readable = QSettings{}.value(Globals::console_human_readable_view_key, false).toBool();
                bool is_connect_signal = signal == &CommunicationDevice::connected;
                bool is_disconnect_signal = signal == &CommunicationDevice::disconnected;
                if (is_connect_signal) {
                    if (display_text.count()) {
                        display_text = "connected(" + display_text + ")";
                    } else {
                        display_text = "connected";
                    }
                    is_human_readable = true;
                }
                if (is_disconnect_signal) {
                    if (display_text.count()) {
                        display_text = "disconnected(" + display_text + ")";
                    } else {
                        display_text = "disconnected";
                    }
                    is_human_readable = true;
                }
                auto display_data = QByteArray();
                display_data.append(display_text);
                MainWindow::mw->append_html_to_console(

                    (fat ? fat_html : normal_html)
                        .arg(QString::number(color.rgb(), 16),
                             is_human_readable ? Utility::to_human_readable_binary_data(display_data) : Utility::to_C_hex_encoding(display_data)),
                    console);
                Utility::thread_call(MainWindow::mw, [console, is_connect_signal, is_disconnect_signal] {
                    if (is_connect_signal || is_disconnect_signal) {
                        QWidget *qt_tabwidget_stackedwidget = console->parentWidget();
                        QWidget *parent_widget = qt_tabwidget_stackedwidget->parentWidget();
                        QTabWidget *tab_widget = dynamic_cast<QTabWidget *>(parent_widget);
                        if (tab_widget) {
                            const auto runner_index = tab_widget->indexOf(console);
                            if (runner_index > -1) {
                                QColor color;
                                if (is_connect_signal) {
                                    color = tab_widget->palette().color(QPalette::Active, QPalette::Text);
                                }
                                if (is_disconnect_signal) {
                                    color = tab_widget->palette().color(QPalette::Disabled, QPalette::Text);
                                }
                                tab_widget->tabBar()->setTabTextColor(runner_index, color);
                            } else {
                                qDebug() << "CommunicationDevice"
                                         << "could not find console" << console;
                            }

                        } else {
                            qDebug() << "CommunicationDevice"
                                     << "could not find tabwidget" << console << qt_tabwidget_stackedwidget << parent_widget << tab_widget;
                        }
                    }
                });
            });
        }
    });
}

std::vector<PortDescription *> DeviceWorker::get_devices_with_protocol(const QString &protocol, const QStringList device_names) {
    try {
        return Utility::promised_thread_call(this, [this, protocol, device_names]() mutable {
            assert(currently_in_devices_thread());
            std::vector<PortDescription *> candidates;
            for (auto &device : communication_devices) { //TODO: do not only loop over comport_devices, but other devices as well
                if (not device.protocol) {
                    continue;
                }
                if (not device.device) {
                    continue;
                }
                if (not device.device->isConnected()) {
                    continue;
                }
                bool device_name_match = false;
                QString device_name;
                if (auto protocol = dynamic_cast<SCPIProtocol *>(device.protocol.get())) {
                    device_name = QString::fromStdString(protocol->get_name());
                } else if (auto protocol = dynamic_cast<RPCProtocol *>(device.protocol.get())) {
                    device_name = QString::fromStdString(protocol->get_name());
                } else if (auto protocol = dynamic_cast<ManualProtocol *>(device.protocol.get())) {
                    device_name = QString::fromStdString(protocol->get_name());
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
    } catch (std::runtime_error &e) {
        qDebug() << e.what();
    }
    return {};
}

void DeviceWorker::set_currently_running_test(CommunicationDevice *com_device, const QString &test_name) {
    Utility::promised_thread_call(this, [&] {
        for (auto &device : communication_devices) {
            if (device.device.get() == com_device) {
                device_worker_set_in_use(device.device.get(), not test_name.isEmpty());
                MainWindow::mw->execute_in_gui_thread([item = device.ui_entry, test_name] { item->setText(3, test_name); });
                break;
            }
        }
    });
}

QStringList DeviceWorker::get_string_list(ScriptEngine &script, const QString &name) {
    return Utility::promised_thread_call(this, [&script, &name] { return script.get_string_list(name); });
}

bool DeviceWorker::contains_port(QMap<QString, QVariant> port_info) {
    assert(currently_in_devices_thread());
    for (const auto &item : communication_devices) {
        if (port_info[HOST_NAME_TAG].toString() == "manual") {
            if ((port_info[DEVICE_MANUAL_NAME_TAG].toString() == item.port_info[DEVICE_MANUAL_NAME_TAG].toString()) &&
                (port_info[DEVICE_MANUAL_SN_TAG].toString() == item.port_info[DEVICE_MANUAL_SN_TAG].toString())) {
                return true;
            }
        } else {
            if (port_info[HOST_NAME_TAG].toString() == item.port_info[HOST_NAME_TAG].toString()) {
                return true;
            }
        }
    }
    return false;
}
