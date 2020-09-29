#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

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

class MovingAverage {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    MovingAverage(number average_time_ms);
#endif
    /// \cond HIDDEN_SYMBOLS
    MovingAverage(QPlainTextEdit *console, double average_time_ms);
    ~MovingAverage();
    /// \endcond
    // clang-format off
/*! \fn MovingAverage(number average_time_ms);
    \brief Creates a MovingAverage object for writing and calculating a moving average value. The buffe will contain values from now-average_time_ms till now.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 750
    sleep_ms(2.5*1000)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 1000
\endcode

*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    append(number value);
#endif
    /// \cond HIDDEN_SYMBOLS
       void append(double value);
    /// \endcond
    // clang-format off
/*! \fn append(number value);
    \brief Appends the value together with the timestamp 'now' to the moving average buffer.
        At the same time it deletes the values which are older than now-average_time_ms.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 750
    sleep_ms(2.5*1000)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 1000
\endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_average();
#endif
    /// \cond HIDDEN_SYMBOLS
    double get_average();
    /// \endcond
    // clang-format off
/*! \fn number get_average();
    \brief returns the current average value

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 750
    sleep_ms(2.5*1000)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 1000
\endcode
*/

    double get_count();
    double get_min();
    double get_max();
    double get_stddev();
    void clear();

    bool is_empty();
    private:
    /// \cond HIDDEN_SYMBOLS
    double m_average_time_ms;
    QPlainTextEdit *console;
    QMap<qint64,double> m_average_storage;
    /// \endcond
};

/** \} */ // end of group convenience
#endif // MOVING_AVERAGE_H
