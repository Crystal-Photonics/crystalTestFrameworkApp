#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QString>
#include <sol.hpp>
#include <QStringList>

enum class LoggerSaveFormat {none, csv} ;
enum class LoggerFieldFormat {string,num} ;

class DataLogger
{

public:
    DataLogger(std::string filename, std::string format, char seperating_character, sol::table field_names);
    ~DataLogger();
    void append_data(sol::table data_record);
    void save();
    void set_save_interval(int save_interaval);


private:

    QString filename;
    QList<QStringList> data_to_save;
    QList<LoggerFieldFormat> field_formats;
    int auto_save_interval=10;
    LoggerSaveFormat format=LoggerSaveFormat::csv;
    QChar seperating_character;

    void dump_data_to_file();

};

#endif // DATALOGGER_H
