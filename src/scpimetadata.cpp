#include "scpimetadata.h"

#include "mainwindow.h"
#include <QDate>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <string.h>

SCPIMetaData::SCPIMetaData() {}

void SCPIMetaData::reload(QString file_name) {
    parse_meta_data_file(file_name);
}

SCPIDeviceType SCPIMetaData::query(QString serial_number, QString device_name) {
    bool look_for_serial;
    SCPIDeviceType result{};

    if (serial_number.count()) {
        look_for_serial = true;
    } else {
        look_for_serial = false;
    }
    for (auto t : device_types) {
        if (look_for_serial) {
            for (auto d : t.devices) {
                if (d.serial_number == serial_number) {
                    result = t;
                    result.devices.clear();
                    result.devices.append(d);
                    break;
                }
            }
        } else {
            if (t.device_name == device_name) {
                result = t;
                break;
            }
        }
    }
    return result;
}

void SCPIMetaData::parse_meta_data_file(QString file_name) {
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
        SCPIDeviceType device_type{};

        QJsonObject obj = js_device_type.toObject();
       // device_type.isEmpty = false;
        device_type.device_name = obj["device_name"].toString();
        device_type.manual_path = obj["manual_path"].toString();
        QJsonArray js_devices = obj["devices"].toArray();

        for (QJsonValue js_device : js_devices) {
            SCPIDeviceDetail device{};
            const QString DATE_FORMAT = "yyyy.M.d"; // 1985.5.25
            QJsonObject obj = js_device.toObject();
            device.serial_number = obj["serial_number"].toString();
            QString tmp = obj["expery_date"].toString();

            device.expery_date = QDate::fromString(tmp, DATE_FORMAT);
            tmp = obj["purchase_date"].toString();
            device.purchase_date = QDate::fromString(tmp, DATE_FORMAT);

            device.locked = obj["locked"].toBool();
            device.note = obj["note"].toString();
            device.calibration_certificate_path = obj["calibration_certificate_path"].toString();
            device_type.devices.append(device);
        }
        device_types.append(device_type);
    }
}

void SCPIDeviceType::clear()
{
    device_name = "";
    manual_path  = "";
    //isEmpty = false;
    devices.clear();
}
