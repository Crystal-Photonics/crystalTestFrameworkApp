#include "userentrystorage.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QObject>
#include <QSettings>

UserEntryCache::UserEntryCache() {
    clear();
}

void UserEntryCache::clear() {
    cache_m = QJsonObject{};
    modified_m = false;
    dut_id_m = "";
    read_from_wild_card = false;
    modified_m = false;
    file_name_m = "";
    script_path_m = "";
}

UserEntryCache::~UserEntryCache() {
    if (modified_m) {
        if (file_name_m == "") {
            return;
        }
        QJsonObject obj{};
        obj["cache"] = cache_m;
        obj["script_path"] = script_path_m;
        obj["touched_unixtime"] = QJsonValue(qint64(QDateTime::currentDateTime().toTime_t()));

        QJsonDocument saveDoc(obj);
        QFile saveFile(file_name_m);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            MainWindow::mw->show_status_bar_massage(QObject::tr("Could not save user entry cache file.") + file_name_m, 2000);
            return;
        }
        saveFile.write(saveDoc.toJson());
    }
}

void UserEntryCache::load_storage_for_script(QString script_path, QString dut_id) {
    dut_id_m = dut_id;
    script_path_m = script_path;
    QString root_path = QSettings{}.value(Globals::user_entry_cache_path_key, "").toString();
    if (root_path == "") {
        file_name_m = "";
        return;
    }
    file_name_m = script_path;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray ba;
    ba.append(script_path);
    hash.addData(QByteArray(ba));

    file_name_m = root_path + "/" + hash.result().toHex() + ".json";

    QFile file;

    if (!QFile::exists(file_name_m)) {
        return;
    }

    file.setFileName(file_name_m);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QString json_string = file.readAll();
    file.close();

    QJsonDocument j_doc = QJsonDocument::fromJson(json_string.toUtf8());
    if (j_doc.isNull()) {
        MainWindow::mw->show_status_bar_massage(QObject::tr("Could not parse  user entry cache file. Seems the json is broken: ") + file_name_m, 2000);
        return;
    }

    QJsonObject j_obj = j_doc.object();
    cache_m = j_obj["cache"].toObject();
    read_from_wild_card = !cache_m.contains(dut_id_m);
    modified_m = false;
}

QString UserEntryCache::get_value(QString field_name) {
    if (read_from_wild_card) {
        return cache_m[wildcard].toObject()["content"].toObject()[field_name].toString();
    } else {
        return cache_m[dut_id_m].toObject()["content"].toObject()[field_name].toString();
    }
}

void UserEntryCache::set_value(QString field_name, QString value) {
    auto keys = QStringList{dut_id_m, wildcard};
    for (auto key : keys) {
        cache_m[key].toObject()["content"].toObject()[field_name] = value;
        cache_m[key].toObject()["touched_unixtime"] = QJsonValue(qint64(QDateTime::currentDateTime().toTime_t()));
    }
    modified_m = true;
}

bool UserEntryCache::key_already_used(AccessDirection access_direction, QString field_name) {
    (void)access_direction;
    (void)field_name;
    return false;
}
