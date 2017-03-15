#include "scpiprotocol.h"
#include "config.h"
#include "console.h"
#include "qt_util.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTreeWidgetItem>
#include <cassert>
#include <fstream>

using namespace std::chrono_literals;

static void set_display_data(QTreeWidgetItem *item, const SCPI_Device_Data &data) {
    const auto &summary = data.get_summary();
    item->setToolTip(0, summary);
    item->addChild(new QTreeWidgetItem(item, QStringList{} << summary));
}

QString SCPI_Device_Data::get_summary() const {
    QStringList statustip;
    for (auto &d : get_description_source()) {
        if (d.source.isEmpty() == false) {
            statustip << d.description + ": " + d.source;
        }
    }
    return statustip.join("\n");
}

void SCPI_Device_Data::get_lua_data(sol::table &t) const {
    for (auto &d : get_description_source()) {
        t.set(d.description.toStdString(), d.source.toStdString());
    }
}

std::vector<SCPI_Device_Data::Description_source> SCPI_Device_Data::get_description_source() const {
    return {{"Manufacturer", manufacturer}, {"Name", name}, {"Serialnumber", serial_number}, {"Version", version}};
}

SCPIProtocol::SCPIProtocol(CommunicationDevice &device)
    : Protocol{"SCPI"}
    , device(&device) {
#if 1
    connection = QObject::connect(&device, &CommunicationDevice::received, [incoming_data = &incoming_data](const QByteArray &data) {
        //qDebug() << "SCPI-Protocol received" << data.size() << "bytes from device";
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

QStringList SCPIProtocol::scpi_parse_scpi_answers() {
    QStringList result{};
    QString answer_string{incoming_data};
    incoming_data.clear();
    answer_string = answer_string.trimmed();
    result = answer_string.split(QString::fromStdString(escape_characters));
    qDebug() << result;
    return result;
}

QString SCPIProtocol::scpi_parse_last_scpi_answer() {
    QStringList answers = scpi_parse_scpi_answers();
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
    if (true) { //descriptor_answer
        ui_entry->setText(1, "SCPI");
        ui_entry->setText(2, device_data.name);
        set_display_data(ui_entry, device_data);
    } else {
        Console::note() << "SCPI-request \"*IDN?\" did not answer";
    }
}

void SCPIProtocol::get_lua_device_descriptor(sol::table &t) const {
    return device_data.get_lua_data(t);
}

void SCPIProtocol::clear() {}

bool SCPIProtocol::is_correct_protocol() {
    bool result = false;
    const CommunicationDevice::Duration TIMEOUT = std::chrono::milliseconds{300};
    for (int i = 0; i < 3; i++) {
        switch (i) {
            case 0:
                escape_characters = "\r\n";
                break;
            case 1:
                escape_characters = "\r";
                break;
            case 2:
                escape_characters = "\n";
                break;
        }
        if (send_scpi_request(TIMEOUT, "*IDN?", true)) {
            result = true;
            QString answer = scpi_parse_last_scpi_answer();
            qDebug() << answer;

            load_idn_string(answer.toStdString());
            break;
        }
    }
    return result;
}

void SCPIProtocol::send_string(std::string data) {
    Utility::thread_call(device, [ device = this->device, data = data ] {
        std::string display_data = QString::fromStdString(data).trimmed().toStdString();
        std::vector<unsigned char> data_vec{data.begin(), data.end()};
        std::vector<unsigned char> disp_vec{display_data.begin(), display_data.end()};
        device->send(data_vec, disp_vec);
    });
}

bool SCPIProtocol::send_scpi_request(Duration timeout, std::string request, bool use_leading_escape) {
    bool cancel_request = false;
    bool success = false;
    request = request + escape_characters;
    if (use_leading_escape) {
        send_string(escape_characters);
        if (device->waitReceived(timeout, escape_characters, event_indicator) == false) {
            //cancel_request = false; wont work with hameg
        }
    }
    if (!cancel_request) {
        //retrieve_events();
        //incoming_data.clear();
        send_string(request);
        success = device->waitReceived(timeout, escape_characters, event_indicator);
    }
    return success;
}

void SCPIProtocol::load_idn_string(std::string idn) {
    //parses eg. "Agilent Technologies,U1252B,serial_number,V2.18\r\n"
    //parses eg. "HAMEG Instruments,HM8150,1.12\r"
    QString q_idn{QString::fromStdString(idn)};
    q_idn = q_idn.trimmed();
    QStringList idn_nodes = q_idn.split(",");
    if (idn_nodes.count() == 4) {
        device_data.manufacturer = idn_nodes[0];
        device_data.name = idn_nodes[1];
        device_data.serial_number = idn_nodes[2];
        device_data.version = idn_nodes[3];
    } else if (idn_nodes.count() == 3) {
        device_data.manufacturer = idn_nodes[0];
        device_data.name = idn_nodes[1];
        device_data.version = idn_nodes[2];
        device_data.serial_number.clear();
    }
}

QStringList SCPIProtocol::scpi_get_str_param_raw(std::string request, std::string argument) {
    QStringList result{};
    if (argument.empty()) {
        request = request + "?";
    } else {
        request = request + "?" + " " + argument;
    }
    bool timeout_ok = send_scpi_request(default_duration, request, false);
    if (timeout_ok) {
        QString str_to_parse = scpi_parse_last_scpi_answer();
        QStringList answer_items = str_to_parse.split(",");
        for (auto str : answer_items) {
            str = str.trimmed();
            result.append(str);
        }
    }
    return result;
}

sol::table SCPIProtocol::scpi_get_str_param(sol::state &lua, std::string request, std::string argument) {
    QStringList sl = scpi_get_str_param_raw(request, argument);
    sol::table result = lua.create_table_with();
    if (sl.count()) {
        for (auto str : sl) {
            str = str.trimmed();
            result.add(str.toStdString());
        }
    }
    return result;
}

double SCPIProtocol::scpi_get_num_param(std::string request, std::string argument) {
    QStringList sl = scpi_get_str_param_raw(request, argument);
    bool ok = false;
    double result = 0;
    if (sl.count()) {
        result = sl[0].toFloat(&ok);
    }
    return result;
}

bool SCPIProtocol::is_event_received(std::string event_name) {
    device->waitReceived(default_duration, escape_characters, "");
    scpi_parse_last_scpi_answer();
    return event_list.indexOf(QString::fromStdString(event_name)) >= 0;
}

sol::table SCPIProtocol::get_event_list(sol::state &lua) {
    device->waitReceived(default_duration, escape_characters, "");
    scpi_parse_last_scpi_answer();
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

std::string SCPIProtocol::get_manufacturer() {
    return device_data.manufacturer.toStdString();
}

double SCPIProtocol::scpi_get_num(std::string request) {
    return scpi_get_num_param(request, "");
}

sol::table SCPIProtocol::scpi_get_str(sol::state &lua, std::string request) {
    return scpi_get_str_param(lua, request, "");
}
