#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <sol_forward.hpp>
/** \ingroup convenience
 *  \{
 */
// clang-format off

/*!
    \class   MovingAverage
    \brief A MovingAverage object is useful to calculate statistical properties of numerical values checked in within the last \c average_time_ms milliseconds.
    It contains a buffer which maps each value to a timestamp. Whenever a new value is checked in using the \c append function, values which are older
    than \c average_time_ms will be deleted. This way you can use the object to calculate e.g the average, stddev, max or min value of the last
    \c average_time_ms quite conveniently. Read access will not delete any value from the buffer.
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
    \brief Creates a MovingAverage object for writing and calculating a moving average value. The buffer will contain values of the last \c average_time_ms milliseconds.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 750
    sleep_ms(2.5*1000)
    avg:append(1000)        --appends 1000 and deletes
                            --500 and 1000 because they
                            --are older than 2s
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
        At the same time it deletes the values older than \c average_time_ms milliseconds.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 750
    sleep_ms(2.5*1000)
    avg:append(1000)        --appends 1000 and deletes
                            --500 and 1000 because they
                            --are older than 2s
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
    \brief Returns the current average value calculated by:
         \f[
        get\_average = \frac{1}{get\_count}\sum_{n=1}^{get\_count} value_n
          \f]
     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_average()
    print(result) --should be 750
    sleep_ms(2.5*1000)
    avg:append(1000)        --appends 1000 and deletes
                            --500 and 1000 because they
                            --are older than 2s
    local result = avg:get_average()
    print(result) --should be 1000
\endcode
*/


#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_count();
#endif
    /// \cond HIDDEN_SYMBOLS
    double get_count();
    /// \endcond
    // clang-format off
/*! \fn     number get_count();
    \brief Returns how many values the internal buffer contains.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_count()
    print(result) --should be 2
    sleep_ms(2.5*1000)
    avg:append(1000)        --appends 1000 and deletes
                            --500 and 1000 because they
                            --are older than 2s
    local result = avg:get_count()
    print(result) --should be 1
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_min();
    number get_max();
#endif
    /// \cond HIDDEN_SYMBOLS
    double get_min();
    double get_max();
    /// \endcond
    // clang-format off
/*! \fn     number get_min();
    \fn     number get_max();
    \brief Returns the min/max value of the internal buffer.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_min()
    print(result) --should be 500
    sleep_ms(2.5*1000)
    avg:append(2000)        --appends 2000 and deletes
                            --500 and 1000 because they
                            --are older than 2s
    local result = avg:get_min()
    print(result) --should be 2000
\endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number get_stddev();
#endif
    /// \cond HIDDEN_SYMBOLS
    double get_stddev();
    /// \endcond
    // clang-format off
/*! \fn     number get_stddev();
    \brief Returns the current standard deviation calculated by:

         \f[
        get\_stddev = \sqrt{\frac{1}{get\_count}\sum_{n=1}^{get\_count} \left(value_n - get\_average \right)^2}
          \f]

*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    clear();
#endif
    /// \cond HIDDEN_SYMBOLS
    void clear();
    /// \endcond
    // clang-format off
/*! \fn     clear();
    \brief Removes all values from internal buffer.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    avg:append(500)
    avg:append(1000)
    local result = avg:get_count()
    print(result) --should be 2
    avg:clear()
    local result = avg:get_count()
    print(result) --should be 0
\endcode
*/
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_empty();
#endif
    /// \cond HIDDEN_SYMBOLS
    bool is_empty();
    /// \endcond
    // clang-format off
/*! \fn     bool is_empty();
    \brief Returns false if buffer contains values, otherwise true.

     \par examples:
     \code
    avg = MovingAverage.new(2*1000)
    local result = avg:is_empty()
    print(result) --should be true
    avg:append(1000)
    local result = avg:is_empty()
    print(result) --should be false

\endcode
*/

    private:
    /// \cond HIDDEN_SYMBOLS
    double m_average_time_ms;
    QPlainTextEdit *console;
    QMap<qint64,double> m_average_storage;
    /// \endcond
};

/** \} */ // end of group convenience
#endif // MOVING_AVERAGE_H
