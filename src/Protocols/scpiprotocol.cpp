#include "scpiprotocol.h"
#include "Windows/mainwindow.h"
#include "Windows/scpimetadatadeviceselector.h"
#include "config.h"
#include "console.h"
#include "qt_util.h"

#include "CommunicationDevices/usbtmccommunicationdevice.h"
#include <QByteArray>
#include <QDateTime>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTranslator>
#include <QTreeWidgetItem>
#include <cassert>
#include <cmath>
#include <fstream>

using namespace std::chrono_literals;

static void set_display_data(QTreeWidgetItem *item, SCPI_Device_Data &data) {
    const auto &summary = data.get_summary();
    item->setToolTip(0, summary);
    item->addChild(new QTreeWidgetItem(item, QStringList{} << summary));
}

QString SCPI_Device_Data::get_summary() {
    QStringList statustip;
    auto ds = get_description_source();
    for (auto &d : ds) {
        if (d.source.isEmpty() == false) {
            statustip << d.description + ": " + d.source;
        }
    }
    return statustip.join("\n");
}

void SCPI_Device_Data::get_lua_data(sol::table &t) {
    for (auto &d : get_description_source()) {
        t.set(d.name.toStdString(), d.source.toStdString());
    }
}

DeviceMetaDataApprovedState SCPI_Device_Data::get_approved_state() {
    return approved_state;
}

QString SCPI_Device_Data::get_approved_state_str() {
    return approved_str;
}

void SCPI_Device_Data::set_approved_state(DeviceMetaDataApprovedState as, QString sr) {
    approved_str = sr;
    approved_state = as;
}

std::vector<SCPI_Device_Data::Description_source> SCPI_Device_Data::get_description_source() {
    QString expery_date_str = expery_date.toString("yyyy.MM.dd");
    QString blocked_str = "";
    QString valid_str = "";
    if (approved_state == DeviceMetaDataApprovedState::Unknown) {
        valid_str = "failed";
        expery_date_str = "";
    } else {
        valid_str = "ok";
        blocked_str = approved_str;
        if (!expery_date.isValid()) {
            expery_date_str = "never";
        }
    }

    return {
        {"Manufacturer", "Manufacturer", manufacturer},                            //
        {"Name", "Name", name},                                                    //
        {"Serialnumber", "Serialnumber", serial_number},                           //
        {"Version", "Version", version},                                           //
        {"Calibration_expery_date", "Calibration expery date", expery_date_str},   //
        {"Calibration_approved_state", "Calibration approved state", blocked_str}, //
        {"Metadata_found", "Metadata found", valid_str}                            //
    };
}

SCPIProtocol::SCPIProtocol(CommunicationDevice &device, DeviceProtocolSetting &setting)
    : Protocol{"SCPI"}
    , device(&device)
    , device_protocol_setting(setting) {
#if 1
    connection = QObject::connect(&device, &CommunicationDevice::received, [incoming_data = &incoming_data](const QByteArray &data) {
        //qDebug() << "SCPI-Protocol received" << data.size() << "bytes from device";
        //qDebug() << "SCPI-Protocol received" << data << "bytes from device";
        incoming_data->append(data);
    });
#endif
    assert(connection);
}

SCPIProtocol::~SCPIProtocol() {
    assert(connection);
    auto result = QObject::disconnect(connection);
    assert(result);
}

QStringList SCPIProtocol::parse_scpi_answers() {
    QStringList result{};
    QString answer_string{incoming_data};
    incoming_data.clear();
    answer_string = answer_string.trimmed();
    result = answer_string.split(QString::fromStdString(escape_characters));
    if (answer_string.count()) {
        //  qDebug() << result;
    }
    return result;
}

QString SCPIProtocol::parse_last_scpi_answer() {
    QStringList answers = parse_scpi_answers();
    for (auto s : answers) {
        if (s.startsWith(QString::fromStdString(event_indicator))) {
            if (event_list.indexOf(s) == -1) {
                event_list.append(s);
            }
        }
    }
    return answers.last();
}

void SCPIProtocol::set_ui_description(QTreeWidgetItem *ui_entry) {
    assert(QThread::currentThread() == MainWindow::gui_thread);
    if (true) { //descriptor_answer
        ui_entry->setText(1, "SCPI");
        ui_entry->setText(2, device_data.name);
        set_display_data(ui_entry, device_data);
    } else {
        Console::note() << "SCPI-request \"*IDN?\" did not answer";
    }
}

void SCPIProtocol::get_lua_device_descriptor(sol::table &t) {
    return device_data.get_lua_data(t);
}

QString SCPIProtocol::get_device_summary() {
    return device_data.get_summary();
}

void SCPIProtocol::clear() {}

void SCPIProtocol::set_scpi_meta_data(DeviceMetaDataGroup scpi_meta_data) {
    DeviceMetaDataDetail scpi_device_detail{};

    if (scpi_meta_data.devices.count() == 1) {
        if (device_data.serial_number == "") {
            assert(device_data.name == scpi_meta_data.commondata.device_name);
        } else {
            assert(device_data.serial_number == scpi_meta_data.devices[0].serial_number);
        }
        scpi_device_detail = scpi_meta_data.devices[0];

    } else if (scpi_meta_data.devices.count() == 0) {
		Utility::thread_call(MainWindow::mw, [ comportname = device->getName(), serial_number = device_data.serial_number, device_name = device_data.name ] {
			QMessageBox::warning(MainWindow::mw, "Unknown SCPI device",
								 QString("Could not find SCPI-Device in meta data table. Serialnumber: \"%1\" name: \"%2\" @%3 .")
									 .arg(serial_number)
									 .arg(device_name)
									 .arg(comportname));

		});

    } else {
		Utility::promised_thread_call(MainWindow::mw, [ comportname = device->getName(), &scpi_meta_data ]() mutable {
            SCPIMetaDataDeviceSelector meta_data_selector(MainWindow::mw);
            meta_data_selector.load_devices(comportname, scpi_meta_data);
            meta_data_selector.exec();
            scpi_meta_data = meta_data_selector.get_device();
        });

        if (scpi_meta_data.devices.count() == 1) {
            scpi_device_detail = scpi_meta_data.devices[0];
        }
    }

    device_data.set_approved_state(scpi_device_detail.get_approved_state(), scpi_device_detail.get_approved_state_str());
    if (scpi_device_detail.valid) {
        device_data.serial_number = scpi_meta_data.devices[0].serial_number;
        device_data.expery_date = scpi_meta_data.devices[0].expery_date;
        device_data.purchase_date = scpi_meta_data.devices[0].purchase_date;
        device_data.manual_path = scpi_meta_data.commondata.manual_path;
        device_data.calibration_certificate_path = scpi_meta_data.devices[0].calibration_certificate_path;
        device_data.note = scpi_meta_data.devices[0].note;
        device_data.locked = scpi_meta_data.devices[0].locked;
    }
}

bool SCPIProtocol::is_correct_protocol() {
    bool result = false;
    const CommunicationDevice::Duration TIMEOUT = device_protocol_setting.timeout;

    escape_characters = device_protocol_setting.escape.toStdString();
    bool use_leading_escape = true;
	USBTMCCommunicationDevice *usbtmctest = dynamic_cast<USBTMCCommunicationDevice *>(device);
	if (usbtmctest) {
        use_leading_escape = false; //tmc devices not need leading escape
    }
    if (send_scpi_request(TIMEOUT, "*IDN?", use_leading_escape, true)) {
        result = true;
        QString answer = parse_last_scpi_answer();
        //qDebug() << answer;

        load_idn_string(answer.toStdString());
    }

    return result;
}

void SCPIProtocol::send_string(std::string data) {
	Utility::promised_thread_call(device, [ device = this->device, data = data ] {
        std::string display_data = QString::fromStdString(data).trimmed().toStdString();
        std::vector<unsigned char> data_vec{data.begin(), data.end()};
        std::vector<unsigned char> disp_vec{display_data.begin(), display_data.end()};
        device->send(data_vec, disp_vec);
    });
}

QString clean_up_regex_with_escape_characters(QString expr) {
    QString result = expr.replace("*", "\\*");
    result = result.replace("?", "\\?");
    result = result.replace("|", "\\|");
    result = result.replace("@", "\\@");
    result = result.replace("#", "\\#");
    result = result.replace(".", "\\.");
    return result;
}

bool SCPIProtocol::send_scpi_request(Duration timeout, std::string request, bool use_leading_escape, bool answer_expected) {
    bool cancel_request = false;
    bool success = false;
    request = request + escape_characters;
    device->set_currently_in_wait_received(true);
    QString event{QString().fromStdString(event_indicator)};
    event = clean_up_regex_with_escape_characters(event);
    QString request_regex = clean_up_regex_with_escape_characters(QString().fromStdString(request));
    event = "^(" + request_regex + "|" + event + ")";

    if (use_leading_escape) {
        send_string(escape_characters);
        if (answer_expected) {
            if (device->waitReceived(timeout, escape_characters, event.toStdString()) == false) {
                //cancel_request = false; wont work with hameg
            }
        }
        //we need to wait here for some devices
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(device_protocol_setting.pause_after_discovery_flush).count();
        if (ms) {
            QThread::currentThread()->msleep(ms);
        }
    }
    if (!cancel_request) {
        //retrieve_events();
        //incoming_data.clear();
        send_string(request);
        if (answer_expected) {
            success = device->waitReceived(timeout, escape_characters, event.toStdString());
        }
    }
    if (!answer_expected) {
        success = true;
    }
    return success;
}

void SCPIProtocol::send_command(std::string request) {
    send_scpi_request(std::chrono::milliseconds(0), request, false, false);
}

void SCPIProtocol::load_idn_string(std::string idn) {
    //parses eg. "Agilent Technologies,U1252B,serial_number,V2.18\r\n"
    //parses eg. "HAMEG Instruments,HM8150,1.12\r"
    QString q_idn{QString::fromStdString(idn)};
    q_idn = q_idn.trimmed();
    QStringList idn_nodes = q_idn.split(",");
    if (idn_nodes.count() == 4) {
        device_data.manufacturer = idn_nodes[0].trimmed();
        device_data.name = idn_nodes[1].trimmed();
        device_data.serial_number = idn_nodes[2].trimmed();
        device_data.version = idn_nodes[3];
    } else if (idn_nodes.count() == 3) {
        device_data.manufacturer = idn_nodes[0].trimmed();
        device_data.name = idn_nodes[1].trimmed();
        device_data.version = idn_nodes[2].trimmed();
        device_data.serial_number.clear();
    } else if (idn_nodes.count() == 2) {
        device_data.manufacturer = "";
        device_data.name = idn_nodes[0].trimmed();
        device_data.version = idn_nodes[1].trimmed();
        device_data.serial_number.clear();
    }
}

QStringList SCPIProtocol::get_str_param_raw(std::string request, std::string argument) {
    QStringList result{};
    if (argument.empty()) {
        request = request + "?";
    } else {
        request = request + "?" + " " + argument;
    }

    bool timeout_ok = send_scpi_request(device_protocol_setting.timeout, request, false, true);
    if (timeout_ok) {
        QString str_to_parse = parse_last_scpi_answer();
        QStringList answer_items = str_to_parse.split(",");
        for (auto str : answer_items) {
            str = str.trimmed();
            result.append(str);
        }
    } else {
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(device_protocol_setting.timeout).count();
        qDebug() << "timeout " << ms << "ms received for " + QString().fromStdString(request);
    }
    return result;
}

sol::table SCPIProtocol::get_str_param(sol::state &lua, std::string request, std::string argument) {
    QStringList sl = get_str_param_raw(request, argument);
    sol::table result = lua.create_table_with();
    if (sl.count()) {
        for (auto str : sl) {
            str = str.trimmed();
            result.add(str.toStdString());
        }
    }

    return result;
}

double SCPIProtocol::get_num_param(std::string request, std::string argument) {
    QList<double> values{};
    double mean = 0;
    for (int i = 0; i < retries_per_transmission + 1; i++) {
        QStringList sl = get_str_param_raw(request, argument);
        bool ok = false;
        double result = 0;
        if (sl.count()) {
            QString s = sl[0];
            QStringList colon_seperated_list = s.split(":"); //sometimes we receive a "FRQ:10.000E+0" and just want the number
            s = colon_seperated_list.last().trimmed();
            while (s[0].isLetter()) { //sometimes we receive a "10.00160V" but we just want the number
                s = s.remove(0, 1);
            }
            while (s.right(1)[0].isLetter()) {
                s = s.remove(s.size() - 1, 1);
            }
            s = s.trimmed();
            result = s.toDouble(&ok);
            if (ok && s.count()) {
                values.append(result);
                mean += result;
            }
        }
    }
    mean /= values.count();
    double standard_deviation = 0;
    if (values.count() == retries_per_transmission + 1) {
        for (auto d : values) {
            standard_deviation += std::pow(d - mean, 2.0);
        }
        standard_deviation /= values.count();
        standard_deviation = std::sqrt(standard_deviation);
        if (standard_deviation > maximal_acceptable_standard_deviation) {
            //TODO put variance error here
        }
        // qDebug() << "SCPI standard deviation: " << standard_deviation;
    } else {
        //TODO put conversion error
        qDebug() << "SCPI conversion error";
    }
    return mean;
}

double SCPIProtocol::get_num(std::string request) {
    return get_num_param(request, "");
}

bool SCPIProtocol::is_event_received(std::string event_name) {
    // device->waitReceived(std::chrono::milliseconds(0), escape_characters, "");
    // scpi_parse_last_scpi_answer();
    return event_list.indexOf(QString::fromStdString(event_name)) >= 0;
}

sol::table SCPIProtocol::get_event_list(sol::state &lua) {
    // device->waitReceived(default_duration, escape_characters, "");
    //    device->waitReceived(std::chrono::milliseconds(0), escape_characters, "");
    parse_last_scpi_answer();
    sol::table result = lua.create_table_with();
    for (auto str : event_list) {
        str = str.trimmed();
        result.add(str.toStdString());
    }
    return result;
}

void SCPIProtocol::clear_event_list() {
    event_list.clear();
}

std::string SCPIProtocol::get_name() {
    return device_data.name.toStdString();
}

std::string SCPIProtocol::get_serial_number() {
    return device_data.serial_number.toStdString();
}

DeviceMetaDataApprovedState SCPIProtocol::get_approved_state() {
    return device_data.get_approved_state();
}

QString SCPIProtocol::get_approved_state_str() {
    return device_data.get_approved_state_str();
}

std::string SCPIProtocol::get_manufacturer() {
    return device_data.manufacturer.toStdString();
}

void SCPIProtocol::set_validation_retries(unsigned int retries) {
    retries_per_transmission = retries;
}

void SCPIProtocol::set_validation_max_standard_deviation(double max_std_dev) {
    maximal_acceptable_standard_deviation = max_std_dev;
}

sol::table SCPIProtocol::get_str(sol::state &lua, std::string request) {
    return get_str_param(lua, request, "");
}
