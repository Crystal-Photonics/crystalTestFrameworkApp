#include "datalogger.h"
#include "Windows/mainwindow.h"
#include "console.h"
#include "qt_util.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include <sol.hpp>

DataLogger::DataLogger(QPlainTextEdit *console, std::string filename, char separating_character, sol::table field_names, bool over_write_file) {
    this->filename = QString::fromStdString(filename);
    this->format = LoggerSaveFormat::csv;
    this->console = console;
    if ((this->filename == "")) {
        const auto &message = QObject::tr("Filename is empty.");
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("Filename is empty");
    }

    if (!over_write_file) {
        if (QFile::exists(this->filename)) {
            const auto &message = QObject::tr("File for saving csv already exists and must not be overwritten: %1").arg(this->filename);
            Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
            throw sol::error("File already exists");
        }
    }

    QFile file(this->filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const auto &message = QObject::tr("File for saving csv can not be opened: %1").arg(this->filename);
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("Cannot open file");
    }
    file.close();

    this->separating_character = separating_character;
    QStringList fns;
    field_formats.clear();
    for (auto &field_name : field_names) {
        QString fn = QString::fromStdString(field_name.second.as<std::string>());
        fns.append(fn);
        field_formats.append(LoggerFieldFormat::string);
    }
    data_to_save.append(fns);
    save();
}

DataLogger::~DataLogger() {
    dump_data_to_file();
}

void DataLogger::append_data(sol::table data_record) {
    QStringList record{};
    for (auto &field : data_record) {
        if (field.second.get_type() == sol::type::string) {
            QString field_entry = QString::fromStdString(field.second.as<std::string>());
            QString s = QString(separating_character) + QString(separating_character);
            field_entry = field_entry.replace(separating_character, s);
            field_entry = "\"" + field_entry + "\"";
            record.append(field_entry);
        } else if (field.second.get_type() == sol::type::number) {
            QString numStr;
            numStr = QString::number(field.second.as<double>(), 'f', 10);
            if (numStr.indexOf(".") > 0) {
                while (numStr.endsWith("0")) {
                    numStr.remove(numStr.count() - 1, 1);
                }
                if (numStr.endsWith(".")) {
                    numStr.remove(numStr.count() - 1, 1);
                }
            }
            record.append(numStr);
        }
    }
    data_to_save.append(record);
    if (data_to_save.count() > auto_save_interval) {
        dump_data_to_file();
    }
}

void DataLogger::save() {
    dump_data_to_file();
}

void DataLogger::dump_data_to_file() {
    if (data_to_save.count()) {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            const QString fname = QFileInfo(filename).fileName();
            auto dot_pos = fname.lastIndexOf('.');
            QString file_template;
            const auto &file_path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/CrystalTestFramework Emergency Log";
            QDir{}.mkdir(file_path);
            file_template = file_path + '/' + (dot_pos == -1 ? fname + "_XXXXXX" : fname.left(dot_pos) + "_XXXXXX" + fname.mid(dot_pos));
            QTemporaryFile tempfile{file_template};
            tempfile.open();
            tempfile.setAutoRemove(false);
            tempfile.close();
            QString new_filename = tempfile.fileName();
            assert(not new_filename.isEmpty());
            Utility::thread_call(MainWindow::mw, [old_filename = this->filename, new_filename, console = console] {
                const auto message =
                    QString{"Data is supposed to be logged to file\n%1\nHowever, this file is currently not available. The logging will continue in file\n%2"}
                        .arg(old_filename)
                        .arg(new_filename);
                Console_handle::warning(console) << message;
                QMessageBox::warning(MainWindow::mw, "CrystalTestFramework - IO Error", message);
            });
            filename = new_filename;
            return dump_data_to_file();
        }
        QTextStream out(&file);
        for (auto &record : data_to_save) {
            QString record_s = record.join(separating_character);
            out << record_s << "\n";
        }
        data_to_save.clear();
    }
}
