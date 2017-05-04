#include "scpimetadata.h"
#include "Windows/mainwindow.h"

#include <QDate>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <string.h>

DeviceMetaData::DeviceMetaData() {}

DeviceMetaDataApprovedState DeviceMetaDataDetail::get_approved_state() {
    if (valid == false) {
        return DeviceMetaDataApprovedState::Unknown;
    }
    if (locked) {
        return DeviceMetaDataApprovedState::Locked;
    }
    if ((QDateTime().currentDateTime() > QDateTime(expery_date)) && (expery_date.isValid())) {
        return DeviceMetaDataApprovedState::Expired;
    }

    return DeviceMetaDataApprovedState::Approved;
}

QString DeviceMetaDataDetail::get_approved_state_str() {
    switch (get_approved_state()) {
        case DeviceMetaDataApprovedState::Approved:
            if (expery_date.isValid()) {
                return "calibrated until " + expery_date.toString("yyyy.MM.dd");
            } else {
                return "calibration approved";
            }
        case DeviceMetaDataApprovedState::Unknown:
            return "unknown";

        case DeviceMetaDataApprovedState::Locked:
            return "locked";

        case DeviceMetaDataApprovedState::Expired:
            return "calibration expired";
    }
}

void DeviceMetaData::reload(QString file_name) {
    parse_meta_data_file(file_name);
}

DeviceMetaDataGroup DeviceMetaData::query(QString serial_number, QString device_name) {
    bool look_for_serial;
    DeviceMetaDataGroup result{};

    if (serial_number.count()) {
        look_for_serial = true;
    } else {
        look_for_serial = false;
    }
    for (auto t : device_types) {
        if (look_for_serial) {
            for (auto d : t.devices) {
                if (d.serial_number == serial_number) {
                    if (device_name == t.commondata.device_name) {
                        result = t;
                        result.devices.clear();
                        result.devices.append(d);
                        break;
                    }
                }
            }
        } else {
            if (t.commondata.device_name == device_name) {
                result = t;
                break;
            }
        }
    }
    return result;
}

QList<DeviceEntry> DeviceMetaData::get_manual_devices() const {
    QList<DeviceEntry> result;
    for (auto &devgroup : device_types) {
        if (devgroup.commondata.protocol_type != ProtocolType::Manual) {
            continue;
        }
        for (auto &detail : devgroup.devices) {
            DeviceEntry entry;
            entry.detail = detail;
            entry.commondata = devgroup.commondata;
            result.append(entry);
        }
    }
    return result;
}

void DeviceMetaData::parse_meta_data_file(QString file_name) {
    device_types.clear();

    QFile file;

    file.setFileName(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "Can't measurement equipment meta data file", "Can't measurement equipment meta data file: " + file_name);
        });

        return;
    }
    QString json_string = file.readAll();
    file.close();
    QJsonDocument j_doc = QJsonDocument::fromJson(json_string.toUtf8());
    if (j_doc.isNull()) {
        Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, "measurement equipment meta data parse error",
                                 "could not parse measurement equipment meta data file: " + file_name);
        });
        return;
    }
    QJsonObject j_obj = j_doc.object();
    QJsonArray js_device_types = j_obj["device_types"].toArray();
    for (QJsonValue js_device_type : js_device_types) {
        DeviceMetaDataGroup device_type{};

        QJsonObject obj = js_device_type.toObject();
        // device_type.isEmpty = false;
        device_type.commondata.device_name = obj["device_name"].toString();
        device_type.commondata.manufacturer = obj["manufacturer"].toString();
        device_type.commondata.description = obj["description"].toString();
        device_type.commondata.manual_path = obj["manual_path"].toString();
        QString proto_type = obj["protocol"].toString();
        if (proto_type.toLower() == "scpi") {
            device_type.commondata.protocol_type = ProtocolType::SCPI;
        } else if (proto_type.toLower() == "manual") {
            device_type.commondata.protocol_type = ProtocolType::Manual;
        } else {
            //TODO: put error here
            assert(0);
        }

        QJsonArray js_devices = obj["devices"].toArray();

        for (QJsonValue js_device : js_devices) {
            DeviceMetaDataDetail device{};
            const QString DATE_FORMAT = "yyyy.M.d"; // 1985.5.25
            QJsonObject obj = js_device.toObject();
            device.serial_number = obj["serial_number"].toString();
            QString tmp = obj["expery_date"].toString();

            device.expery_date = QDate::fromString(tmp, DATE_FORMAT);
            tmp = obj["purchase_date"].toString();
            device.purchase_date = QDate::fromString(tmp, DATE_FORMAT);
            device.valid = true;
            device.locked = false;

            QString locked_str = obj["locked"].toString();
            if (locked_str == "yes") {
                device.locked = true;
            }
            if (locked_str == "true") {
                device.locked = true;
            }
            int locked_num = obj["locked"].toInt();
            if (locked_num) {
                device.locked = true;
            }
            device.note = obj["note"].toString();
            device.calibration_certificate_path = obj["calibration_certificate_path"].toString();
            device_type.devices.append(device);
        }
        device_types.append(device_type);
    }
}

void DeviceMetaDataGroup::clear() {
    commondata.clear();
    devices.clear();
}

void DeviceMetaDataCommon::clear() {
    device_name = "";
    manual_path = "";
    manufacturer = "";
    description = "";
}
