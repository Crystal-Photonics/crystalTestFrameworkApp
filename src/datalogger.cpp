#include "datalogger.h"
#include "Windows/mainwindow.h"
#include "console.h"
#include "qt_util.h"
#include <QFile>
#include <QTextStream>

DataLogger::DataLogger(QPlainTextEdit *console, std::string filename, char seperating_character, sol::table field_names, bool over_write_file) {
    this->filename = QString::fromStdString(filename);
    this->format = LoggerSaveFormat::csv;
    this->console = console;
    if ((this->filename == "")) {
        const auto &message = QObject::tr("Filename is empty.");
		Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("Filename is empty");
    }

    if (!over_write_file) {
        if (QFile::exists(this->filename)) {
            const auto &message = QObject::tr("File for saving csv already exists and must not be overwritten: %1").arg(this->filename);
			Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
            throw sol::error("File already exists");
        }
    }

    QFile file(this->filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const auto &message = QObject::tr("File for saving csv can not be opened: %1").arg(this->filename);
		Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("Cannot open file");
    }
    file.close();

    this->seperating_character = seperating_character;
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
            QString s = QString(seperating_character) + QString(seperating_character);
            field_entry = field_entry.replace(seperating_character, s);
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
            const auto &message = QObject::tr("File for saving csv can not be opened: for appending %1").arg(this->filename);
			Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
            throw sol::error("Cannot open file");
        }
        QTextStream out(&file);
        for (auto &record : data_to_save) {
            QString record_s = record.join(seperating_character);
            out << record_s << "\n";
        }
        data_to_save.clear();
    }
}
