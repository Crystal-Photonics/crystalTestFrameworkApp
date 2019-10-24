#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <sol_forward.hpp>
/// \cond HIDDEN_SYMBOLS
enum class LoggerSaveFormat { none, csv };
enum class LoggerFieldFormat { string, num };
/// \endcond
/** \ingroup convenience
 *  \{
 */
// clang-format off

/*!
    \class   DataLogger
    \brief A DataLogger object can be used to write data into csv files. CSV files are text-tables where each column is separated by a separation character (e.g ";", "\t").
    The first line contains the column names and the following lines the data. These files are very easy to process by python/Excel/OpenOffice.
*/
// clang-format on

class DataLogger {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    DataLogger(string filename, char separating_character, string_table field_names, bool over_write_file);
#endif
    /// \cond HIDDEN_SYMBOLS
    DataLogger(QPlainTextEdit *console, std::string filename, char separating_character, sol::table field_names, bool over_write_file);
    ~DataLogger();
    /// \endcond
    // clang-format off
/*! \fn DataLogger(string filename, char separating_character, string_table field_names, bool over_write_file);
    \brief Creates a DataLogger object for writing a file to \c filename with the title line \c field_names, using \c seperating_character as column separator.
        \param filename the filename where the csv file is written to.
        \param separating_character the column separator character(";","|" etc.)
        \param field_names A table with the name for each column.
        \param over_write_file if true, the Datalogger object is allowed to overwrite files.
        \sa propose_unique_filename_by_datetime()
     \par examples:
     \code
    local done_button = Ui.Button.new("Stop")
    logger = DataLogger.new("example.csv",";",{"time","offset_mv","temperature_mv"}, true)
    while not done_button:has_been_clicked() do
        if true then
            local offset_mv = math.random()
            local temperature_mv = math.random()
            print(os.time(), "    ", offset_mv , "    ", offset_mv);
            logger:append_data({os.time(),offset_mv,offset_mv})
        end
        sleep_ms(1000)
    end
\endcode
    This code creates a file like this:
\code
    time;offset_mv;temperature_mv
    1571923058;0.0012512207;0.0012512207
    1571923059;0.1932983398;0.1932983398
    1571923060;0.5849914551;0.5849914551
    ..
    ..

\endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    append_data(table data_record);
#endif
    /// \cond HIDDEN_SYMBOLS
       void append_data(sol::table data_record);
    /// \endcond
    // clang-format off
/*! \fn append_data(table data_record);
    \brief Appends a data record to the csv table.
        \param data_record the data set to append. Each element of the data_record corresponds to its column.
     \par examples:
     \code
    local done_button = Ui.Button.new("Stop")
    logger = DataLogger.new("example.csv",";",{"time","offset_mv","temperature_mv"}, true)
    while not done_button:has_been_clicked() do
        if true then
            local offset_mv = math.random()
            local temperature_mv = math.random()
            print(os.time(), "    ", offset_mv , "    ", offset_mv);
            logger:append_data({os.time(),offset_mv,offset_mv})
        end
        sleep_ms(1000)
    end
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    save();
#endif
    /// \cond HIDDEN_SYMBOLS
       void save();
    /// \endcond
    // clang-format off
/*! \fn save();
    \brief Saves the data manually to file. The file is normally written automatically after 10 cycles or when the script ends.

     \par examples:
     \code
    local done_button = Ui.Button.new("Stop")
    logger = DataLogger.new("example.csv",";",{"time","offset_mv","temperature_mv"}, true)
    while not done_button:has_been_clicked() do
        if true then
            local offset_mv = math.random()
            local temperature_mv = math.random()
            logger:append_data({os.time(),offset_mv,offset_mv})
            logger:save()
        end
        sleep_ms(1000)
    end
\endcode
*/



    private:
    /// \cond HIDDEN_SYMBOLS
    QString filename;
    QList<QStringList> data_to_save;
    QList<LoggerFieldFormat> field_formats;
    int auto_save_interval = 10;
    LoggerSaveFormat format = LoggerSaveFormat::csv;
    QChar separating_character;

    void dump_data_to_file();
    QPlainTextEdit *console;
    /// \endcond
};

/** \} */ // end of group convenience
#endif // DATALOGGER_H
