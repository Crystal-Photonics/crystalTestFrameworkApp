#include "device_protocols_settings.h"
#include "Windows/mainwindow.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QObject>
#include <assert.h>
#include <regex>

DeviceProtocolsSettings::DeviceProtocolsSettings(QString file_name) {
    this->file_name = file_name;
    parse_settings_file(file_name);
}

void DeviceProtocolsSettings::parse_settings_file(QString file_name) {
    protocols_rpc.clear();
    protocols_scpi.clear();
    protocols_sg04_count.clear();
    exclusive_ports.clear();

    QStringList protocol_identifiers{"SCPI", "RPC", "SG04Count"};

    QFile file;

	if (file_name == "") {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "Can't open port settings", "Could not open device settings file: " + file_name);
        });

        return;
    }
	if (!QFile::exists(file_name)) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "Can't open port settings", "Could not open device settings file: " + file_name);
        });

        return;
    }

    file.setFileName(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "Can't open port settings", "Could not open device settings file: " + file_name);
        });

        return;
    }
    QString json_string = file.readAll();
    file.close();
    QJsonDocument j_doc = QJsonDocument::fromJson(json_string.toUtf8());
    if (j_doc.isNull()) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "port settings parse error", "could not parse device settings file: " + file_name);
        });
        return;
    }
    bool devices_parsed = false;
    QJsonObject j_obj = j_doc.object();
    for (auto &protocol_identifier : protocol_identifiers) {
        QJsonArray prot_settings = j_obj[protocol_identifier].toArray();
        for (QJsonValue prot_setting : prot_settings) {
            DeviceProtocolSetting setting{};
            setting.set_exclusive_port(&exclusive_ports);

            QJsonObject obj = prot_setting.toObject();

            QString portType = obj["type"].toString().toUpper();
            if (portType == "COM") {
                setting.type = DeviceProtocolSetting::comport;
            } else if (portType == "TMC") {
                setting.type = DeviceProtocolSetting::tmcport;
            } else if (portType == "TCP") {
                setting.type = DeviceProtocolSetting::tcp_connection;
            } else {
				Utility::thread_call(MainWindow::mw, [file_name, portType] {
                    QMessageBox::warning(MainWindow::mw, "unknown protocol in port settings",
                                         "unknown protocol \"" + portType + "\" in port settings device settings file: " + file_name);
                });
                assert(0);
                return;

                //error
            }
            setting.internet_port = obj["internet_port"].toInt();
            setting.ip_address = obj["ip_address"].toString();
            setting.com_port_name_regex = obj["com_port_name_regex"].toString();
            setting.baud = obj["baud"].toInt();
            setting.escape = obj["escape"].toString();
            setting.escape = setting.escape.replace("\\n", "\n");
            setting.escape = setting.escape.replace("\\r", "\r");
            setting.timeout = std::chrono::milliseconds(obj["timeout_ms"].toInt());
            setting.pause_after_discovery_flush = std::chrono::milliseconds(obj["pause_after_discovery_flush_ms"].toInt());
            if (obj["exclusive"].toString() == "yes") {
                exclusive_ports.append(setting.com_port_name_regex);
            }
            if (protocol_identifier == "SCPI") {
                protocols_scpi.append(setting);
                devices_parsed = true;
            } else if (protocol_identifier == "RPC") {
                protocols_rpc.append(setting);
                devices_parsed = true;
            } else if (protocol_identifier == "SG04Count") {
                protocols_sg04_count.append(setting);
                devices_parsed = true;
            }
        }
    }
    if (!devices_parsed) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "no settings in port settings error", "did not find device definitions in device settings file: " + file_name);
        });
        return;
    }
}

void DeviceProtocolSetting::set_exclusive_port(QStringList *exclusive_ports) {
    this->exclusive_ports = exclusive_ports;
}

bool regex_match(QString regex, QString port_name) {
    std::regex port_name_regex{regex.toStdString()};

    std::string port_name_std = port_name.toStdString();
    auto port_name_regex_begin = std::sregex_iterator(port_name_std.begin(), port_name_std.end(), port_name_regex);
    auto port_name_regex_end = std::sregex_iterator();
    int match_count = std::distance(port_name_regex_begin, port_name_regex_end);
    return match_count;
}

bool DeviceProtocolSetting::match(QString port_name) {
    bool matches = regex_match(com_port_name_regex, port_name);
    for (auto s : *exclusive_ports) {
        if (s == com_port_name_regex) {
            continue;
        }
        if (regex_match(s, port_name)) {
            matches = false;
        }
    }
    return matches;
}
