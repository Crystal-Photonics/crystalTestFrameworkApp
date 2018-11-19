#include "favorite_scripts.h"
#include "Windows/mainwindow.h"
#include "qt_util.h"
#include "util.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>

FavoriteScripts::FavoriteScripts() {}

void FavoriteScripts::load_from_file(QString file_name) {
    file_name_m = file_name;
    if (file_name == "") {
        return;
    }
    QFile file;

    if (!QFile::exists(file_name)) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, QObject::tr("Can't open favorite scripte file"),
                                 QObject::tr("Can't open favorite scripte file. File does not exist: ") + file_name);
        });

        return;
    }

    file.setFileName(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, QObject::tr("Can't open favorite scripte file"),
                                 QObject::tr("Can't open favorite scripte file: ") + file_name);
        });

        return;
    }
    QString json_string = file.readAll();
    file.close();
    QJsonDocument j_doc = QJsonDocument::fromJson(json_string.toUtf8());
    if (j_doc.isNull()) {
		Utility::thread_call(MainWindow::mw, [file_name] {
            QMessageBox::warning(MainWindow::mw, QObject::tr("could not parse file with favorite scripts"),
                                 QObject::tr("could not parse file with favorite scripts. Seems the json is broken: ") + file_name);
        });
        return;
    }

    QJsonObject j_obj = j_doc.object();
    const auto array = j_obj["scripts"].toArray();
    for (const auto &val : array) {
        ScriptEntry entry{};
        const auto &obj = val.toObject();
        entry.script_path = obj["script_path"].toString();
        entry.alternative_name = obj["alternative_name"].toString();
        entry.description = obj["description"].toString();
        entry.icon = QIcon{"://src/file_icon.ico"};
        entry.valid = true;
        script_entries_m.append(entry);
    }
}

bool FavoriteScripts::save_to_file(QList<ScriptEntry> script_entries) {
    if (file_name_m == "") {
        return false;
    }
    QJsonArray array{};
    for (const ScriptEntry &entry : script_entries) {
        QJsonObject obj{};
        obj["script_path"] = entry.script_path;
        obj["alternative_name"] = entry.alternative_name;
        obj["description"] = entry.description;
        array.append(obj);
    }
    QJsonObject obj{};
    obj["scripts"] = array;
    QJsonDocument saveDoc(obj);
    QFile saveFile(file_name_m);
    if (!saveFile.open(QIODevice::WriteOnly)) {
		Utility::thread_call(MainWindow::mw, [this] {
            QMessageBox::warning(MainWindow::mw, QObject::tr("Could not save file with favorite scripts"),
                                 QObject::tr("Could not save file for favorite scripts. Is it write protected?\n") + file_name_m);
        });
        return false;
    }
    saveFile.write(saveDoc.toJson());
    return true;
}

ScriptEntry FavoriteScripts::get_entry(QString script_path) {
    int index = get_index_by_path(script_path);
    if (index > -1) {
        return script_entries_m[index];
    }
    ScriptEntry entry{};
    return entry;
}

bool FavoriteScripts::add_favorite(QString script_path) {
    if (!is_favorite(script_path)) {
        ScriptEntry entry{};
        entry.script_path = script_path;
        entry.valid = true;
        QList<ScriptEntry> script_entries = script_entries_m;
        script_entries.append(entry);
        bool result = save_to_file(script_entries);
        if (result) {
            script_entries_m = script_entries;
        }
        return result;
    }
    return true;
}

bool FavoriteScripts::set_alternative_name(QString script_path, QString alternative_name) {
    int index = get_index_by_path(script_path);
    if (index > -1) {
        if (script_entries_m[index].alternative_name != alternative_name) {
            QString old_name = script_entries_m[index].alternative_name;
            script_entries_m[index].alternative_name = alternative_name;
            bool result = save_to_file(script_entries_m);
            if (!result) {
                script_entries_m[index].alternative_name = old_name;
            }
            return result;
        } else {
            return true;
        }
    }
    return true;
}

bool FavoriteScripts::remove_favorite(QString script_path) {
    int index = get_index_by_path(script_path);
    if (index > -1) {
        QList<ScriptEntry> script_entries = script_entries_m;
        script_entries.removeAt(index);
        bool result = save_to_file(script_entries);
        if (result) {
            script_entries_m = script_entries;
        }
        return result;
    }
    return true;
}

bool FavoriteScripts::is_favorite(QString script_path) {
    int index = get_index_by_path(script_path);
    return index > -1;
}

int FavoriteScripts::get_index_by_path(QString script_path) {
    int result = 0;
    for (const ScriptEntry &entry : script_entries_m) {
        if (entry.script_path == script_path) {
            return result;
        }
        result++;
    }
    return -1;
}
