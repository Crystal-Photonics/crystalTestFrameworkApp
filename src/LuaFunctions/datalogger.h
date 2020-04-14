#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <sol_forward.hpp>

enum class LoggerSaveFormat { none, csv };
enum class LoggerFieldFormat { string, num };

class DataLogger {
    public:
    DataLogger(QPlainTextEdit *console, std::string filename, char seperating_character, sol::table field_names, bool over_write_file);
    ~DataLogger();
    void append_data(sol::table data_record);
    void save();
    void set_save_interval(int save_interaval);

    private:
    QString filename;
    QList<QStringList> data_to_save;
    QList<LoggerFieldFormat> field_formats;
    int auto_save_interval = 10;
    LoggerSaveFormat format = LoggerSaveFormat::csv;
    QChar seperating_character;

    void dump_data_to_file();
    QPlainTextEdit *console;
};

#endif // DATALOGGER_H
