#ifndef CHARGECOUNTER_H
#define CHARGECOUNTER_H
#include <QDateTime>

/** \ingroup convenience
 *  \{
 */
// clang-format off

/*!
    \class   ChargeCounter
    \brief A ChargeCounter object can be used to log and integrate currents over time. This is very useful if you want to track a charging/discharge process of a
    battery.
    The output can be retrieved by calling get_current_hours() which returns the integrated charge with the units mAh, Ah or Xh depending on what values were fed into
    the ChargeCounter object.
*/
// clang-format on

class ChargeCounter {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    ChargeCounter();
#endif
    /// \cond HIDDEN_SYMBOLS
    ChargeCounter();
    /// \endcond
    // clang-format off
/*! \fn ChargeCounter();
    \brief Creates a ChargeCounter object.

     \par examples:
     \code
    charge_counter = ChargeCounter.new()
    local done_button = Ui.Button.new("Stop")
    local start_ms=current_date_time_ms()
    while not done_button:has_been_clicked() do
        charge_counter:add_current(3600);
        print("charge [mAh]: "..charge_counter:get_current_hours())
        sleep_ms(1000)          --wait a second
        print("run_time_ms: "..current_date_time_ms()-start_ms)
    end
    \endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    add_current(double current);
#endif
    /// \cond HIDDEN_SYMBOLS
    void add_current(const double current);
    /// \endcond
    // clang-format off
/*! \fn add_current(double current);
    \brief Appends a measurement point together with a timestamp to the integrating buffer. The unit of the current you check in determines the unit of the
    resulting integrated value. If you check-in [mA] values, the resulting unit will be [mAh], if you use [A] the result will be [Ah].
    \param current the current you want to append to the integral.

     \par examples:
     \code
    charge_counter = ChargeCounter.new()
    local done_button = Ui.Button.new("Stop")
    local start_ms=current_date_time_ms()
    while not done_button:has_been_clicked() do
        charge_counter:add_current(3600);
        print("charge [mAh]: "..charge_counter:get_current_hours())
        sleep_ms(1000)          --wait a second
        print("run_time_ms: "..current_date_time_ms()-start_ms)
    end
    \endcode
*/





#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    double get_current_hours();
#endif
    /// \cond HIDDEN_SYMBOLS
    double get_current_hours();
    /// \endcond
    // clang-format off
/*! \fn double get_current_hours();
    \brief Returns the integrated charge as [mAh / Ah / etc]

     \par examples:
     \code
    charge_counter = ChargeCounter.new()
    local done_button = Ui.Button.new("Stop")
    local start_ms=current_date_time_ms()
    while not done_button:has_been_clicked() do
        charge_counter:add_current(3600);
        print("charge [mAh]: "..charge_counter:get_current_hours())
        sleep_ms(1000)          --wait a second
        print("run_time_ms: "..current_date_time_ms()-start_ms)
    end
    \endcode
*/

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    reset();
#endif
    /// \cond HIDDEN_SYMBOLS
    void reset();
    /// \endcond
    // clang-format off
/*! \fn reset();
    \brief Resets the charge counter object to zero.

     \par examples:
     \code
    charge_counter = ChargeCounter.new()
    local done_button = Ui.Button.new("Stop")
    local start_ms=current_date_time_ms()
    while not done_button:has_been_clicked() do
        charge_counter:add_current(3600);
        print("charge [mAh]: "..charge_counter:get_current_hours())
        sleep_ms(1000)          --wait a second
        print("run_time_ms: "..current_date_time_ms()-start_ms)
    end
    charge_counter:reset()
    print("charge [mAh] after reset: "..charge_counter:get_current_hours())
    \endcode
*/


    private:
    /// \cond HIDDEN_SYMBOLS
    double current_hours = 0.0;
    double last_current = 0;
    bool last_current_valid = false;
    QDateTime last_current_date_time;
    /// \endcond
};

/** \} */ // end of group convenience

#endif // CHARGECOUNTER_H
