#include "device_protocols_settings.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <assert.h>

DeviceProtocolsSettings::DeviceProtocolsSettings(QString file_name) {
    this->file_name = file_name;
    parse_settings_file(file_name);
}

void DeviceProtocolsSettings::parse_settings_file(QString file_name) {
    protocols_rpc.clear();
    protocols_scpi.clear();

    QStringList protocol_identifiers{"SCPI", "RPC"};

    QFile file;
    file.setFileName(file_name);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString json_string = file.readAll();
    file.close();

    //qWarning() << json_string;
    QJsonDocument j_doc = QJsonDocument::fromJson(json_string.toUtf8());
    QJsonObject j_obj = j_doc.object();
    for (auto &protocol_identifier : protocol_identifiers) {
        QJsonArray prot_settings = j_obj[protocol_identifier].toArray();
        for (QJsonValue prot_setting : prot_settings) {
            DeviceProtocolSetting setting{};
            QJsonObject obj = prot_setting.toObject();
            QString portType = obj["type"].toString().toUpper();
            if (portType == "COM") {
                setting.type = DeviceProtocolSetting::comport;
            } else if (portType == "TCP") {
                setting.type = DeviceProtocolSetting::tcp_connection;
            } else {
                assert(0);
                //error
            }
            setting.internet_port = obj["internet_port"].toInt();
            setting.ip_address = obj["ip_address"].toString();
            setting.com_port_name_regex = obj["com_port_name_regex"].toString();
            setting.baud = obj["baud"].toInt();
            setting.escape = obj["escape"].toString();
            setting.timeout = std::chrono::milliseconds(obj["timeout_ms"].toInt());
            setting.pause_after_discovery_flush = std::chrono::milliseconds(obj["pause_after_discovery_flush_ms"].toInt());

            if (protocol_identifier == "SCPI") {
                protocols_scpi.append(setting);
            } else if (protocol_identifier == "RPC") {
                protocols_rpc.append(setting);
            }
        }
    }
}
