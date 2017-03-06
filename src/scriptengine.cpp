/// @cond HIDDEN_SYMBOLS
#include "scriptengine.h"
#include "LuaUI/button.h"
#include "LuaUI/lineedit.h"
#include "LuaUI/plot.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "mainwindow.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <vector>

/// @endcond

/**
 * \file   scriptengine.cpp
 * \author Tobias Rieger (tr@crystal-photonics.com),<br> Arne Krüger (ak@crystal-photonics.com)
 * \brief  Lua interface
 * \par
 *  Describes the built-in functions available to the LUA scripts.
 */

/// @cond HIDDEN_SYMBOLS

template <class T>
struct Lua_UI_Wrapper {
    template <class... Args>
    Lua_UI_Wrapper(QSplitter *parent, Args &&... args) {
        Utility::thread_call(MainWindow::mw, [ id = id, parent, args... ] { MainWindow::mw->add_lua_UI_class<T>(id, parent, args...); });
    }
    Lua_UI_Wrapper(Lua_UI_Wrapper &&other)
        : id(other.id) {
        other.id = -1;
    }
    Lua_UI_Wrapper &operator=(Lua_UI_Wrapper &&other) {
        std::swap(id, other.id);
    }
    ~Lua_UI_Wrapper() {
        if (id != -1) {
            Utility::thread_call(MainWindow::mw, [id = this->id] { MainWindow::mw->remove_lua_UI_class<T>(id); });
        }
    }

    int id = id_counter++;

    private:
    static int id_counter;
};

template <class T>
int Lua_UI_Wrapper<T>::id_counter;

namespace detail {
    //this might be replacable by std::invoke once C++17 is available
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs, std::size_t... I>
    ReturnType call_helper(ReturnType (UI_class::*func)(Args...), UI_class &ui, Params<ParamArgs...> const &params, std::index_sequence<I...>) {
        return (ui.*func)(std::get<I>(params)...);
    }
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs, std::size_t... I>
    ReturnType call_helper(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, Params<ParamArgs...> const &params, std::index_sequence<I...>) {
        return (ui.*func)(std::get<I>(params)...);
    }

    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs>
    ReturnType call(ReturnType (UI_class::*func)(Args...), UI_class &ui, Params<ParamArgs...> const &params) {
        return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
    }
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs>
    ReturnType call(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, Params<ParamArgs...> const &params) {
        return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
    }
}

template <class ReturnType, class UI_class, class... Args>
auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...)) {
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
        //TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        return Utility::promised_thread_call(MainWindow::mw, [ function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        });
    };
}

template <class ReturnType, class UI_class, class... Args>
auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...) const) {
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
        //TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        return Utility::promised_thread_call(MainWindow::mw, [ function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        });
    };
}

ScriptEngine::ScriptEngine(QSplitter *parent, QPlainTextEdit *console)
    : lua(std::make_unique<sol::state>())
    , parent(parent)
    , console(console) {}

ScriptEngine::~ScriptEngine() {}

std::string to_string(double d) {
    if (std::fmod(d, 1.) == 0) {
        return std::to_string(static_cast<int>(d));
    }
    return std::to_string(d);
}

std::string to_string(const sol::object &o) {
    switch (o.get_type()) {
        case sol::type::boolean:
            return o.as<bool>() ? "true" : "false";
        case sol::type::function:
            return "function";
        case sol::type::number:
            return to_string(o.as<double>());
        case sol::type::nil:
        case sol::type::none:
            return "nil";
        case sol::type::string:
            return "\"" + o.as<std::string>() + "\"";
        case sol::type::table: {
            auto table = o.as<sol::table>();
            std::string retval{"{"};
            int index = 1;
            for (auto &object : table) {
                auto first_object_string = to_string(object.first);
                if (first_object_string == std::to_string(index++)) {
                    retval += to_string(object.second);
                } else {
                    retval += '[' + std::move(first_object_string) + "]=" + to_string(object.second);
                }
                retval += ", ";
            }
            if (retval.size() > 1) {
                retval.pop_back();
                retval.back() = '}';
                return retval;
            }
            return "{}";
        }
        default:
            return "unknown type " + std::to_string(static_cast<int>(o.get_type()));
    }
}

std::string to_string(const sol::stack_proxy &object) {
    return to_string(sol::object{object});
}

/// @cond HIDDEN_SYMBOLS

std::vector<unsigned int> measure_noise_level_distribute_tresholds(const unsigned int length, const double min_val, const double max_val) {
    std::vector<unsigned int> retval;
    double range = max_val - min_val;
    for (unsigned int i = 0; i < length; i++) {
        double val = round(i * range / length) + min_val;
        retval.push_back(val);
    }
    return retval;
}
/// @endcond
/*! \fn double measure_noise_level_czt(device rpc_device, int dacs_quantity, int max_possible_dac_value)
    \brief Calculates the noise level of an CZT-Detector system with thresholded radioactivity counters.
    \param rpc_device                   The communication instance of the CZT-Detector.
    \param dacs_quantity                Number of thresholds which is also equal to the number of counter results.
    \param max_possible_dac_value       Max digit of the DAC which controlls the thresholds. For 12 Bit this value equals 4095.
    \return                             The lowest DAC threshold value which matches with the noise level definition.

    \details    This function modifies DAC thresholds in order to find the lowest NoiseLevel which matches with:<br>
                \f$ \int_NoiseLevel^\infty spectrum(energy) < LimitCPS \f$ <br>
                where LimitCPS is defined by configuration and is set by default to 5 CPS.
                <br> <br>
                Because this functions iterates through the range of the spectrum to the find the noise level it is
                necessary to have write access to the DAC thresholds and read access to the count values at the
                DAC thresholds.
                This is done by two call back functions which have to be implemented by the user into the lua script: <br><br>

    \par function callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts
    \code{.lua}
            function callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts(
                    rpc_device,
                    thresholds,
                    integration_time_s,
                    count_limit_for_accumulation_abort)
    \endcode
    \brief  Shall modify the DAC thresholds and accumulate the counts of the counters with the new thresholds.
    \param rpc_device                           The communication instance of the CZT-Detector.
    \param thresholds                           Table with the length of \c dacs_quantity containing the thresholds which have to be set by this function.
    \param integration_time_s                   Time in seconds to acquire the counts of the thresholded radioactivity counters.
    \param count_limit_for_accumulation_abort   If not equal to zero the function can abort the count acquisition if one counter reaches
                                                count_limit_for_accumulation_abort.
    \return                                     Table of acquired counts in order of the table \c thresholds.

    \par function callback_measure_noise_level_restore_dac_thresholds_to_normal_mode
    \code{.lua}
        function callback_measure_noise_level_restore_dac_thresholds_to_normal_mode(rpc_device)
    \endcode
    \brief  Modifying thresholds often implies that a special "overwrite with custom thresholds"-mode is required. This function allows the user to leave this mode.
    \param rpc_device                           The communication instance of the CZT-Detector.
    \return                                     nothing.

    \par examples:
    \code{.lua}
        function callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts(
                    rpc_device,
                    thresholds,
                    integration_time_s,
                    count_limit_for_accumulation_abort)

            print("chosen thresholds:", thresholds)
            rpc_device:thresholds_set_custom_values(1 ,
                thresholds[1],
                thresholds[2],
                thresholds[3],
                thresholds[4]) --RPC Function for setting custom thresholds
            local count_values_akku = table_create_constant(DAC_quantity,0)
            rpc_device:get_counts_raw(1)        --RPC Function for resetting
                                                --the internal count buffer
            for i = 1, integration_time_s do    --we want to prevent overflow(16bit) by
                                                --measuring 10 times 1 second instead
                                                --of 10 seconds and add the results
                sleep_ms(1000)
                local count_values = rpc_device:get_counts_raw(1)
                count_values_akku = table_add_table(count_values_akku,count_values)
                if (table_max(count_values_akku) > count_limit_for_accumulation_abort)
                        and (count_limit_for_accumulation_abort ~= 0) then
                    break
                end
            end

            local counts_cps = table_mul_constant(count_values_akku,1/integration_time_s)
            print("measured counts[cps]:", counts_cps)
            return counts_cps
        end
    \endcode
    \code{.lua}
        function callback_measure_noise_level_restore_dac_thresholds_to_normal_mode(rpc_device)
            --eg:
            rpc_device:thresholds_set_custom_values(0, 0,0,0,0) --disable overwriting
                                                                --thresholds with custom values
        end
    \endcode

*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double measure_noise_level_czt(device rpc_device, int dacs_quantity, int max_possible_dac_value);
#endif

/// @cond HIDDEN_SYMBOLS
double measure_noise_level_czt(sol::state &lua, sol::table rpc_device, const unsigned int dacs_quantity, const unsigned int max_possible_dac_value) {
    const unsigned int THRESHOLD_NOISE_LEVEL_CPS = 5;
    const unsigned int INTEGRATION_TIME_SEC = 1;
    const unsigned int INTEGRATION_TIME_HIGH_DEF_SEC = 10;
    double noise_level_result = 100000000;
    //TODO: test if dacs_quantity > 1
    std::vector<unsigned int> dac_thresholds = measure_noise_level_distribute_tresholds(dacs_quantity, 0, max_possible_dac_value);

    for (unsigned int i = 0; i < max_possible_dac_value; i++) {
        sol::table dac_thresholds_lua_table = lua.create_table_with();
        for (auto j : dac_thresholds) {
            dac_thresholds_lua_table.add(j);
        }
        sol::table counts =
            lua["callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts"](rpc_device, dac_thresholds_lua_table, INTEGRATION_TIME_SEC, 0);
        //print counts here
        //for (auto &j : counts) {
        //    double val = std::abs(j.second.as<double>());
        //    qDebug() << val;
        // }
        double window_start = 0;
        double window_end = 0;

        for (int j = counts.size() - 1; j >= 0; j--) {
            if (counts[j + 1].get<double>() > THRESHOLD_NOISE_LEVEL_CPS) {
                window_start = dac_thresholds[j];
                window_end = window_start + (dac_thresholds[1] - dac_thresholds[0]);
                //print("window_start",window_start)
                //print("window_end",window_end)
                break;
            }
        }
        dac_thresholds = measure_noise_level_distribute_tresholds(dacs_quantity, window_start, window_end);

        //print("new threshold:", DAC_THRESHOLDS)

        if (dac_thresholds[0] == dac_thresholds[dacs_quantity - 1]) {
            noise_level_result = dac_thresholds[0];
            break;
        }
    }

    //feinabstung und und Plausibilitätsprüfung
    for (unsigned int i = 0; i < max_possible_dac_value; i++) {
        sol::table dac_thresholds_lua_table = lua.create_table_with();
        ;
        for (unsigned int i = 0; i < dacs_quantity; i++) {
            dac_thresholds_lua_table.add(noise_level_result);
        }
        //print("DAC:",rauschkante)
        sol::table counts = lua["callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts"](
            rpc_device, dac_thresholds_lua_table, INTEGRATION_TIME_HIGH_DEF_SEC, INTEGRATION_TIME_HIGH_DEF_SEC * THRESHOLD_NOISE_LEVEL_CPS);
        bool found = true;
        for (auto &j : counts) {
            double val = j.second.as<double>() / INTEGRATION_TIME_HIGH_DEF_SEC;
            if (val > THRESHOLD_NOISE_LEVEL_CPS) {
                noise_level_result = noise_level_result + 1;
                found = false;
                break;
            }
        }
        if (found) {
            // print("rauschkante gefunden:", noise_level_result);
            break;
        }
    }
    lua["callback_measure_noise_level_restore_dac_thresholds_to_normal_mode"](rpc_device);
    return noise_level_result;
}
/// @endcond

/*! \fn print(argument);
\brief Prints the string value of \c argument to the console.
\param argument             Input value to be printed. Prints all types except for userdefined ones which come the
                            scriptengine. eg. rpc_devices or Ui elements.


\details   \par Differences to the standard Lua world:
  - Besides the normal way you can concatenate arguments using a "," comma. Using multiple arguments.

\par example:
\code{.lua}
    local table_array = {1,2,3,4,5}
    local table_struct = {["A"]=-5, ["B"]=-4,["C"]=-3,["D"]=-2,["E"]=-1}
    local int_val = 536

    print("Hello world")                 -- prints "Hello world"
    print(1.1487)                        -- prints 1.148700
    print(table_array)                   --{1,2,3,4,5}
    print(table_struct)                  -- {["B"]=-4, ["C"]=-3, ["D"]=-2, ["E"]=-1}
    print("outputstruct: ",table_struct) -- "outputstruct: "{["D"]=-2, ["C"]=-3,
                                         --                ["F"]=0, ["E"]=-1}
    print("output int as string: "..int_val) --"output int as string: 536"
    print("output int: ",int_val)        --"output int: "536
    print(non_declared_variable)         --nil
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
print(argument);
#endif

/// @cond HIDDEN_SYMBOLS
void print(QPlainTextEdit *console, const sol::variadic_args &args) {
    std::string text;
    for (auto &object : args) {
        text += to_string(object);
    }
    Utility::thread_call(MainWindow::mw, [ console = console, text = std::move(text) ] { Console::script(console) << text; });
};
/// @endcond

/*! \fn sleep_ms(int timeout_ms);
\brief Pauses the script for \c timeout_ms milliseconds.
\param timeout_ms          Positive integer input value.

\return                    nil

\details    \par example:
\code{.lua}
    local timeout_ms = 100
    sleep_ms(timeout_ms)  -- waits for 100 milliseconds
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
sleep_ms(int timeout_ms);
#endif

/// @cond HIDDEN_SYMBOLS
void sleep_ms(const unsigned int timeout_ms) {
    QEventLoop event_loop;
    static const auto secret_exit_code = -0xF42F;
    QTimer::singleShot(timeout_ms, [&event_loop] { event_loop.exit(secret_exit_code); });
    auto exit_value = event_loop.exec();
    if (exit_value != secret_exit_code) {
        throw sol::error("Interrupted");
    }
};
/// @endcond

/*! \fn double round(double value, int precision);
\brief Returns the rounded value of \c value
\param value                 Input value of int or double values.
\param precision             The number of digits to round.

\return                     the rounded value of \c value.

\details    \par example:
\code{.lua}
    local value = 1.259648
    local retval = round(value,2)  -- retval is 1.26
    local retval = round(value,0)  -- retval is 1.0
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double round(double value, int precision);
#endif

/// @cond HIDDEN_SYMBOLS
double round_double(const double value, const unsigned int precision) {
    double faktor = pow(10, precision);
    double retval = value;
    retval *= faktor;
    retval = round(retval);
    return retval / faktor;
}
/// @endcond

/*! \fn double table_sum(table input_values);
\brief Returns the sum of the table \c input_values
\param input_values                 Input table of int or double values.

\return                     The sum value of \c input_values.

\details    \par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local retval = table_sum(input_values)  -- retval is -28.0
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double table_sum(table input_values);
#endif
/// @cond HIDDEN_SYMBOLS

double table_sum(sol::table input_values) {
    double retval = 0;
    for (auto &i : input_values) {
        retval += i.second.as<double>();
    }
    return retval;
};
/// @endcond

/*! \fn double table_mean(table input_values);
\brief Returns the mean value of the table \c input_values
\param input_values                 Input table of int or double values.

\return                     The mean value of \c input_values.

\details    \par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local retval = table_mean(input_values)  -- retval is -7.0
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double table_mean(table input_values);
#endif
/// @cond HIDDEN_SYMBOLS

double table_mean(sol::table input_values) {
    double retval = 0;
    int count = 0;
    for (auto &i : input_values) {
        retval += i.second.as<double>();
        count += 1;
    }
    if (count) {
        retval /= count;
    }
    return retval;
};

/// @endcond

/*! \fn table table_set_constant(table input_values, double constant);
\brief Returns a table with the length of \c input_values initialized with \c constant.
\param input_values                 Input table of int or double values.
\param constant       Int or double value. The value the array is initialized with.

\return                     A table with the length of \c input_values initialized with \c constant.

\details    \par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local constant = {2.5}
    local retval = table_set_constant(input_values,constant)  -- retval is {2.5, 2.5,
                                                             --          2.5, 2.5}
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_set_constant(table input_values, double constant);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_set_constant(sol::state &lua, sol::table input_values, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        retval.add(constant);
    }
    return retval;
};
/// @endcond

/*! \fn table table_create_constant(int size, double constant);
    \brief Creates a table with \c size elements initialized with \c constant.
    \param size                 Positive integer. The size of the array to be created.
    \param constant              Int or double value. The value the array is initialized with.

    \return                     A table with \c size elements initialized with \c constant.

    \details    \par example:
    \code{.lua}
        local size = 5
        local constant = {2.5}
        local retval = table_create_constant(size,constant)  -- retval is {2.5, 2.5,
                                                       --          2.5, 2.5, 2.5}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_create_constant(int size, double constant);
#endif
/// @cond HIDDEN_SYMBOLS
sol::table table_create_constant(sol::state &lua, const unsigned int size, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= size; i++) {
        retval.add(constant);
    }
    return retval;
};
/// @endcond
///

/*! \fn table table_add_table(table input_values_a, table input_values_b);
    \brief Performs a vector addition of \c input_values_a[i] + \c input_values_b[i] for each i.
    \param input_values_a       Input table of int or double values.
    \param input_values_b       Input table of int or double values. The summands the table \c input_values_a is added with.

    \return                     A table of the differences of \c input_values_a[i] + \c input_values_b[i] for each i.

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local constant = {2}
        local retval = table_add_table(input_values,constant)   -- retval is {-18,
                                                                --   -38, 4, 32}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_add_table(table input_values_a, table input_values_b);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_add_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() + input_values_b[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_add_constant(table input_values,  double constant);
    \brief Performs a vector addition of  \c input_values[i] + \c constant for each i.
    \param input_values       Input table of int or double values.
    \param constant            Int or double value. The summand the table \c input_values is added with.

    \return                     A table of the differences of \c input_values[i] + \c constant for each i.

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_a = {-1.5, 2, 0.5, 3}
        local retval = table_add_constant(input_values,constant) -- retval is {-21.5,
                                                                 --   -38, 2.5, 33}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_add_constant(table input_values, double constant);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_add_constant(sol::state &lua, sol::table input_values, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        double sum_i = input_values[i].get<double>() + constant;
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_sub_table(table input_values_a, table input_values_b);
    \brief Performs a vector subtraction of  \c input_values_a[i] - \c input_values_b[i] for each i.
    \param input_values_a       Input table of int or double values.
    \param input_values_b            Input table of int or double values. The subtrahend the table \c input_values_a is subtracted with.

    \return                     A table of the differences of \c input_values_a[i] - \c input_values_b[i] for each i.

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_a = {-1.5, 2, 0.5, 3}
        local retval = table_sub_table(input_values_a,input_values_b) -- retval is
                                                           --  {-18.5, -42, 1.5, 27}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_sub_table(table input_values_a, table input_values_b);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_sub_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() - input_values_b[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_mul_table(table input_values_a, table input_values_b);
    \brief Performs a vector multiplication of  \c input_values_a[i] * \c input_values_b[i] for each i.
    \param input_values_a       Input table of int or double values.
    \param input_values_b       Input table of int or double values. The factors the table \c input_values_a is multiplied with.

    \return                     A table of the products of \c input_values_a[i] * \c input_values_b[i] for each i.

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_a = {-1.5, 2, 0.5, 3}
        local retval = table_mul_table(input_values_a,input_values_b) -- retval is
                                                            -- {30, -80 , 1, 90}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_mul_table(table input_values_a, table input_values_b);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_mul_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() * input_values_b[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_mul_constant(table input_values_a, double constant);
    \brief Performs a vector multiplication of  \c input_values_a[i] * \c constant for each i.
    \param input_values_a       Input table of int or double values.
    \param constant             Int or double value. The factor the table \c input_values_a is multiplied with.

    \return                     A table of the products of \c input_values_a[i] * \c constant.

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local constant = 3
        local retval = table_mul_constant(input_values_a,constant) -- retval is {-60,
                                                                   --     -120, 6, 90}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_mul_constant(table input_values_a, double constant);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_mul_constant(sol::state &lua, sol::table input_values_a, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() * constant;
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_div_table(table input_values_a, table input_values_b);
    \brief Performs a vector division of  \c input_values_a[i] / \c input_values_b[i] for each i.
    \param input_values_a     Input table of int or double values. Regarded as divident of the division operation.
    \param input_values_b     Input table of int or double values. Regarded as divisor of the division operation.

    \return                 A table of the quotients of \c input_values_a[i] / \c input_values_b[i].

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_b = {5, 2, 8, 10}
        local retval = table_div_table(input_values_a,input_values_b) -- retval is
                                                                      -- {-4, -20,
                                                                      --  0.25, 3}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_div_table(table input_values_a, table input_values_b);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_div_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double c = input_values_b[i].get<double>();
        double sum_i = 0;
        if (c == 0) {
            sum_i = std::numeric_limits<double>::infinity();
        } else {
            sum_i = input_values_a[i].get<double>() / c;
        }

        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_round(table input_values, int precision);
    \brief Returns a table with rounded values of \c input_values
    \param input_values     Input table of int or double values.
    \param precision        The number of digits to round

    \return                 A table of rounded values of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20.35756, -40.41188, 2.1474314, 30.247764}
        local retval_0 = table_round(input_values,2)  -- retval_0 is {-20, -40 ,2, 30}
        local retval_2 = table_round(input_values,2)  -- retval_2 is {-20.36, -40.41,
                                                                   -- 2.15, 30.25}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_round(table input_values, int precision);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_round(sol::state &lua, sol::table input_values, const unsigned int precision) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        double sum_i = round_double(input_values[i].get<double>(), precision);
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_abs(table input_values);
    \brief Returns absolute values of \c input_values.
    \param input_values     Input table of int or double values.

    \return                 Absolute values of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_abs(input_values)  -- retval is {20,40,2,30}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_abs(table input_values);
#endif
/// @cond HIDDEN_SYMBOLS

sol::table table_abs(sol::state &lua, sol::table input_values) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        double sum_i = std::abs(input_values[i].get<double>());
        retval.add(sum_i);
    }
    return retval;
};
/// @endcond

/*! \fn table table_mid(table input_values, int start, int length);
    \brief Returns a fragment of the table \c input_values
    \param input_values     Input table of int or double values.
    \param start            Start index of the fragment of \c input_values which will be returned.
    \param length           Length of the fragment which will be returned.

    \return                 A fragment of the table \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30,25,8,68,42}
        local retval = table_mid(input_values,2,3)  -- retval is {-40,2,30}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
table table_mid(table input_values, int start, int length);
#endif
/// @cond HIDDEN_SYMBOLS
sol::table table_mid(sol::state &lua, sol::table input_values, const unsigned int start, const unsigned int length) {
    sol::table retval = lua.create_table_with();
    for (size_t i = start; i <= start + length - 1; i++) {
        double sum_i = input_values[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
};

/// @endcond

/*! \fn bool table_equal_constant(table input_values_a, double input_const_val);
    \brief Returns true if \c input_values_a[i] == input_const_val for all \c i.
    \param input_values_a                A table of double or int values.
    \param input_const_val               A double value to compare the table with

    \return                            true or false

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_b = {-20, -20, -20, -20}
        local input_const_val = -20
        local retval_equ = table_equal_constant(input_values_b,input_const_val) -- true
        local retval_neq = table_equal_constant(input_values_a,input_const_val) -- false
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
bool table_equal_constant(table input_values_a, double input_const_val);
#endif
/// @cond HIDDEN_SYMBOLS
bool table_equal_constant(sol::table input_values_a, double input_const_val) {
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        if (input_values_a[i].get<double>() != input_const_val) { //TODO: fix double comparison
            return false;
        }
    }
    return true;
};
/// @endcond

/*! \fn bool table_equal_table(table input_values_a, table input_values_b);
    \brief Returns true if \c input_values_a[i] == input_values_b[i] for all \c i.
    \param input_values_a                A table of double or int values.
    \param input_values_b                A table of double or int values.

    \return                            true or false

    \details    \par example:
    \code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_b = {-20, -40, 2, 30}
        local input_values_c = {-20, -40, 2, 31}
        local retval_equ = table_equal_table(input_values_a,input_values_b)  -- true
        local retval_neq = table_equal_table(input_values_a,input_values_c)  -- false
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
bool table_equal_table(table input_values_a, table input_values_b);
#endif
/// @cond HIDDEN_SYMBOLS
bool table_equal_table(sol::table input_values_a, sol::table input_values_b) {
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        if (input_values_a[i].get<double>() != input_values_b[i].get<double>()) { //TODO: fix double comparison
            return false;
        }
    }
    return true;
};
/// @endcond

/*! \fn double table_max(table input_values);
    \brief Returns the maximum value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The maximum absolute value of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_max(input_values)  -- retval is 30
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double table_max(table input_values);
#endif
/// @cond HIDDEN_SYMBOLS
double table_max(sol::table input_values) {
    double max = 0;
    bool first = true;
    for (auto &i : input_values) {
        double val = i.second.as<double>();
        if ((val > max) || first) {
            max = val;
        }
        first = false;
    }
    return max;
};
/// @endcond

/*! \fn double table_min(table input_values);
    \brief Returns the minimum value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The minimum absolute value of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_min(input_values)  -- retval is -40
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double table_min(table input_values);
#endif

/// @cond HIDDEN_SYMBOLS
double table_min(sol::table input_values) {
    double min = 0;
    bool first = true;
    for (auto &i : input_values) {
        double val = i.second.as<double>();
        if ((val < min) || first) {
            min = val;
        }
        first = false;
    }
    return min;
};
/// @endcond

/*! \fn double table_max_abs(table input_values);
    \brief Returns the maximum absolute value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The maximum absolute value of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_max_abs(input_values)  -- retval is 40
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double table_max_abs(table input_values);
#endif

/// @cond HIDDEN_SYMBOLS
double table_max_abs(sol::table input_values) {
    double max = 0;
    bool first = true;
    for (auto &i : input_values) {
        double val = std::abs(i.second.as<double>());
        if ((val > max) || first) {
            max = val;
        }
        first = false;
    }
    return max;
};
/// @endcond

/*! \fn double table_min_abs(table input_values);
    \brief Returns the minimum absolute value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The minimum absolute value of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, 2, 30}
        local retval = table_min_abs(input_values)  -- retval is 2
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double table_min_abs(table input_values);
#endif

/// @cond HIDDEN_SYMBOLS
double table_min_abs(sol::table input_values) {
    double min = 0;
    bool first = true;
    for (auto &i : input_values) {
        double val = std::abs(i.second.as<double>());
        if ((val < min) || first) {
            min = val;
        }
        first = false;
    }
    return min;
}
/// @endcond

void ScriptEngine::load_script(const QString &path) {
    //NOTE: When using lambdas do not capture `this` or by reference, because it breaks when the ScriptEngine is moved
    this->path = path;

    try {
        //load the standard libs if necessary
        lua->open_libraries();

        //add generic function
        {
            (*lua)["show_warning"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
                MainWindow::mw->show_message_box(QString::fromStdString(title.value_or("nil")) + " from " + path,
                                                 QString::fromStdString(message.value_or("nil")), QMessageBox::Warning);
            };
            (*lua)["print"] = [console = console](const sol::variadic_args &args) {
                print(console, args);
            };

            (*lua)["sleep_ms"] = [](const unsigned int timeout_ms) { sleep_ms(timeout_ms); };

            (*lua)["round"] = [](const double value, const unsigned int precision = 0) { return round_double(value, precision); };
        }
        //table functions
        {
            (*lua)["table_sum"] = [](sol::table table) { return table_sum(table); };

            (*lua)["table_mean"] = [](sol::table table) { return table_mean(table); };

            (*lua)["table_set_constant"] = [&lua = *lua](sol::table input_values, double constant) {
                return table_set_constant(lua, input_values, constant);
            };

            (*lua)["table_create_constant"] = [&lua = *lua](const unsigned int size, double constant) {
                return table_create_constant(lua, size, constant);
            };

            (*lua)["table_add_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_add_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_add_constant"] = [&lua = *lua](sol::table input_values, double constant) {
                return table_add_constant(lua, input_values, constant);
            };

            (*lua)["table_sub_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_sub_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_mul_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_mul_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_mul_constant"] = [&lua = *lua](sol::table input_values_a, double constant) {
                return table_mul_constant(lua, input_values_a, constant);
            };

            (*lua)["table_div_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_div_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_round"] = [&lua = *lua](sol::table input_values, const unsigned int precision = 0) {
                return table_round(lua, input_values, precision);
            };

            (*lua)["table_abs"] = [&lua = *lua](sol::table input_values) {
                return table_abs(lua, input_values);
            };

            (*lua)["table_mid"] = [&lua = *lua](sol::table input_values, const unsigned int start, const unsigned int length) {
                return table_mid(lua, input_values, start, length);
            };

            (*lua)["table_equal_constant"] = [](sol::table input_values_a, double input_const_val) {
                return table_equal_constant(input_values_a, input_const_val);
            };

            (*lua)["table_equal_table"] = [](sol::table input_values_a, sol::table input_values_b) {
                return table_equal_table(input_values_a, input_values_b);
            };

            (*lua)["table_max"] = [](sol::table input_values) { return table_max(input_values); };

            (*lua)["table_min"] = [](sol::table input_values) { return table_min(input_values); };

            (*lua)["table_max_abs"] = [](sol::table input_values) { return table_max_abs(input_values); };

            (*lua)["table_min_abs"] = [](sol::table input_values) { return table_min_abs(input_values); };
        }

        {
            (*lua)["measure_noise_level_czt"] = [&lua = *lua](sol::table rpc_device, const unsigned int dacs_quantity,
                                                              const unsigned int max_possible_dac_value) {
                return measure_noise_level_czt(lua, rpc_device, dacs_quantity, max_possible_dac_value);
            };
        }

        //bind UI
        auto ui_table = lua->create_named_table("Ui");

        //bind plot
        {
			ui_table.new_usertype<Lua_UI_Wrapper<Curve>>(
				"Curve",                                                                                //
				sol::meta_function::construct, sol::no_constructor,                                     //
				"append_point", thread_call_wrapper<void, Curve, double, double>(&Curve::append_point), //
				"add_spectrum",
				[](Lua_UI_Wrapper<Curve> &curve, sol::table table) {
					std::vector<double> data;
					data.reserve(table.size());
					for (auto &i : table) {
						data.push_back(i.second.as<double>());
					}
					Utility::thread_call(MainWindow::mw, [ id = curve.id, data = std::move(data) ] {
						auto &curve = MainWindow::mw->get_lua_UI_class<Curve>(id);
						curve.add(data);
					});
				}, //
				"add_spectrum_at",
				[](Lua_UI_Wrapper<Curve> &curve, const unsigned int spectrum_start_channel, const sol::table &table) {
					std::vector<double> data;
					data.reserve(table.size());
					for (auto &i : table) {
						data.push_back(i.second.as<double>());
					}
					Utility::thread_call(MainWindow::mw, [ id = curve.id, data = std::move(data), spectrum_start_channel ] {
						auto &curve = MainWindow::mw->get_lua_UI_class<Curve>(id);
						curve.add_spectrum_at(spectrum_start_channel, data);
					});
				}, //

				"clear",
				thread_call_wrapper(&Curve::clear),                                            //
				"set_median_enable", thread_call_wrapper(&Curve::set_median_enable),           //
				"set_median_kernel_size", thread_call_wrapper(&Curve::set_median_kernel_size), //
				"integrate_ci", thread_call_wrapper(&Curve::integrate_ci),                     //
				"set_x_axis_gain", thread_call_wrapper(&Curve::set_x_axis_gain),               //
				"set_x_axis_offset",
				thread_call_wrapper(&Curve::set_x_axis_offset),                      //
				"set_color_by_name", thread_call_wrapper(&Curve::set_color_by_name), //
				"set_color_by_rgb", thread_call_wrapper(&Curve::set_color_by_rgb),   //
				"user_pick_x_coord",
				[](const Lua_UI_Wrapper<Curve> &lua_curve) {
					QThread *thread = QThread::currentThread();
					std::promise<double> x_selection_promise;
					std::future<double> x_selection_future = x_selection_promise.get_future();
					Utility::thread_call(MainWindow::mw, [&lua_curve, thread, x_selection_promise = &x_selection_promise ]() mutable {
						Curve &curve = MainWindow::mw->get_lua_UI_class<Curve>(lua_curve.id);
						curve.set_onetime_click_callback([thread, x_selection_promise](double x, double y) mutable {
							x_selection_promise->set_value(x);
							Utility::thread_call(thread, [thread] { thread->exit(1234); });
						});
					});
					if (QEventLoop{}.exec() == 1234) {
						return x_selection_future.get();
					} else {
						throw sol::error("aborted");
					}
				}
				//
				);
            ui_table.new_usertype<Lua_UI_Wrapper<Plot>>("Plot",                                                                                          //
                                                        sol::meta_function::construct, [parent = this->parent] { return Lua_UI_Wrapper<Plot>{parent}; }, //
                                                        "clear",
                                                        thread_call_wrapper(&Plot::clear), //
                                                        "add_curve",
                                                        [parent = this->parent](Lua_UI_Wrapper<Plot> & lua_plot)->Lua_UI_Wrapper<Curve> {
                                                            return Utility::promised_thread_call(MainWindow::mw,
                                                                                                 [parent, &lua_plot] {
                                                                                                     auto &plot =
                                                                                                         MainWindow::mw->get_lua_UI_class<Plot>(lua_plot.id);
                                                                                                     return Lua_UI_Wrapper<Curve>{parent, &plot};
                                                                                                 } //
                                                                                                 );
                                                        });
        }
        //bind button
        {
            ui_table.new_usertype<Lua_UI_Wrapper<Button>>("Button", //
                                                          sol::meta_function::construct,
                                                          [parent = this->parent](const std::string &title) {
                                                              return Lua_UI_Wrapper<Button>{parent, title};
                                                          }, //
                                                          "has_been_pressed",
                                                          thread_call_wrapper(&Button::has_been_pressed) //
                                                          );
        }
        //bind edit field
        {
            ui_table.new_usertype<Lua_UI_Wrapper<LineEdit>>(
                "LineEdit",                                                                                          //
                sol::meta_function::construct, [parent = this->parent] { return Lua_UI_Wrapper<LineEdit>(parent); }, //
                "set_placeholder_text", thread_call_wrapper(&LineEdit::set_placeholder_text),                        //
                "get_text", thread_call_wrapper(&LineEdit::get_text),                                                //
                "set_text", thread_call_wrapper(&LineEdit::set_text),                                                //
                "set_name", thread_call_wrapper(&LineEdit::set_name),                                                //
                "get_name", thread_call_wrapper(&LineEdit::get_name),                                                //
                "get_number", thread_call_wrapper(&LineEdit::get_number),                                            //

                "await_return",
                [](const Lua_UI_Wrapper<LineEdit> &lew) {
                    auto le = MainWindow::mw->get_lua_UI_class<LineEdit>(lew.id);
                    le.set_single_shot_return_pressed_callback([thread = QThread::currentThread()] { thread->exit(); });
					QEventLoop{}.exec();
                    auto text = Utility::promised_thread_call(MainWindow::mw, [&le] { return le.get_text(); });
                    return text;
                } //
                );
        }
        lua->script_file(path.toStdString());
    } catch (const sol::error &error) {
        set_error(error);
        throw;
    }
}

void ScriptEngine::set_error(const sol::error &error) {
    const std::string &string = error.what();
    std::regex r(R"(\.lua:([0-9]*): )");
    std::smatch match;
    if (std::regex_search(string, match, r)) {
        Utility::convert(match[1].str(), error_line);
    }
}

void ScriptEngine::launch_editor(QString path, int error_line) {
    auto editor = QSettings{}.value(Globals::lua_editor_path_settings_key, R"(C:\Qt\Tools\QtCreator\bin\qtcreator.exe)").toString();
    auto parameters = QSettings{}.value(Globals::lua_editor_parameters_settings_key, R"(%1)").toString().split(" ");
    for (auto &parameter : parameters) {
        parameter = parameter.replace("%1", path).replace("%2", QString::number(error_line));
    }
    QProcess::startDetached(editor, parameters);
}

void ScriptEngine::launch_editor() const {
    launch_editor(path, error_line);
}

sol::table ScriptEngine::create_table() {
    return lua->create_table_with();
}

QStringList ScriptEngine::get_string_list(const QString &name) {
    QStringList retval;
    sol::table t = lua->get<sol::table>(name.toStdString());
    try {
        if (t.valid() == false) {
            return retval;
        }
        for (auto &s : t) {
            retval << s.second.as<std::string>().c_str();
        }
    } catch (const sol::error &error) {
        set_error(error);
        throw;
    }
    return retval;
}

static sol::object create_lua_object_from_RPC_answer(const RPCRuntimeDecodedParam &param, sol::state &lua) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            auto array = param.as_array();
            if (array.size() == 1) {
                return create_lua_object_from_RPC_answer(array.front(), lua);
            }
            auto table = lua.create_table_with();
            for (auto &element : array) {
                table.add(create_lua_object_from_RPC_answer(element, lua));
            }
            return table;
        }
        case RPCRuntimeParameterDescription::Type::character:
            throw sol::error("TODO: Parse return value of type character");
        case RPCRuntimeParameterDescription::Type::enumeration:
            return sol::make_object(lua.lua_state(), param.as_enum().value);
        case RPCRuntimeParameterDescription::Type::structure: {
            auto table = lua.create_table_with();
            for (auto &element : param.as_struct()) {
                table[element.name] = create_lua_object_from_RPC_answer(element.type, lua);
            }
            return table;
        }
        case RPCRuntimeParameterDescription::Type::integer:
            return sol::make_object(lua.lua_state(), param.as_integer());
    }
    assert(!"Invalid type of RPCRuntimeParameterDescription");
    return sol::nil;
}
/// @cond HIDDEN_SYMBOLS
static void set_runtime_parameter(RPCRuntimeEncodedParam &param, sol::object object) {
    if (param.get_description()->get_type() == RPCRuntimeParameterDescription::Type::array && param.get_description()->as_array().number_of_elements == 1) {
        return set_runtime_parameter(param[0], object);
    }
    switch (object.get_type()) {
        case sol::type::boolean:
            param.set_value(object.as<bool>() ? 1 : 0);
            break;
        case sol::type::function:
            throw sol::error("Cannot pass an object of type function to RPC");
        case sol::type::number:
            param.set_value(object.as<int64_t>());
            break;
        case sol::type::nil:
        case sol::type::none:
            throw sol::error("Cannot pass an object of type nil to RPC");
        case sol::type::string:
            param.set_value(object.as<std::string>());
            break;
        case sol::type::table: {
            sol::table t = object.as<sol::table>();
            if (t.size()) {
                for (std::size_t i = 0; i < t.size(); i++) {
                    set_runtime_parameter(param[i], t[i + 1]);
                }
            } else {
                for (auto &v : t) {
                    set_runtime_parameter(param[v.first.as<std::string>()], v.second);
                }
            }
            break;
        }
        default:
            throw sol::error("Cannot pass an object of unknown type " + std::to_string(static_cast<int>(object.get_type())) + " to RPC");
    }
}

struct RPCDevice {
    sol::object call_rpc_function(const std::string &name, const sol::variadic_args &va) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            throw sol::error("Abort Requested");
        }

        Console::note() << QString("\"%1\" called").arg(name.c_str());
        auto function = protocol->encode_function(name);
        int param_count = 0;
        for (auto &arg : va) {
            auto &param = function.get_parameter(param_count++);
            set_runtime_parameter(param, arg);
        }
        if (function.are_all_values_set()) {
            auto result = protocol->call_and_wait(function);

            if (result) {
                try {
                    auto output_params = result->get_decoded_parameters();
                    if (output_params.empty()) {
                        return sol::nil;
                    } else if (output_params.size() == 1) {
                        return create_lua_object_from_RPC_answer(output_params.front(), *lua);
                    }
                    //else: multiple variables, need to make a table
                    return sol::make_object(lua->lua_state(), "TODO: Not Implemented: Parse multiple return values");
                } catch (const sol::error &e) {
                    Console::error() << e.what();
                    throw;
                }
            }
            throw sol::error("Call to \"" + name + "\" failed due to timeout");
        }
        //not all values set, error
        throw sol::error("Failed calling function, missing parameters");
    }
    sol::state *lua = nullptr;
    RPCProtocol *protocol = nullptr;
    CommunicationDevice *device = nullptr;
    ScriptEngine *engine = nullptr;
};

void add_enum_type(const RPCRuntimeParameterDescription &param, sol::state &lua) {
    if (param.get_type() == RPCRuntimeParameterDescription::Type::enumeration) {
        const auto &enum_description = param.as_enumeration();
        auto table = lua.create_named_table(enum_description.enum_name);
        for (auto &value : enum_description.values) {
            table[value.name] = value.to_int();
            table["to_string"] = [enum_description](int enum_value_param) -> std::string {
                for (const auto &enum_value : enum_description.values) {
                    if (enum_value.to_int() == enum_value_param) {
                        return enum_value.name;
                    }
                }
                return "";
            };
        }
    }
}

void add_enum_types(const RPCRuntimeFunction &function, sol::state &lua) {
    for (auto &param : function.get_request_parameters()) {
        add_enum_type(param, lua);
    }
    for (auto &param : function.get_reply_parameters()) {
        add_enum_type(param, lua);
    }
}

/// @endcond

void ScriptEngine::run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices) {
    auto reset_lua_state = [this] {
        lua = std::make_unique<sol::state>();
        load_script(path);
    };
    try {
        {
            auto device_list = lua->create_table_with();
            for (auto &device_protocol : devices) {
                if (auto rpcp = dynamic_cast<RPCProtocol *>(device_protocol.second)) {
                    device_list.add(RPCDevice{&*lua, rpcp, device_protocol.first, this});
                    auto type_reg = lua->create_simple_usertype<RPCDevice>();
                    for (auto &function : rpcp->get_description().get_functions()) {
                        const auto &function_name = function.get_function_name();
                        type_reg.set(function_name,
                                     [function_name](RPCDevice &device, const sol::variadic_args &va) { return device.call_rpc_function(function_name, va); });
                        add_enum_types(function, *lua);
                    }
                    const auto &type_name = "RPCDevice_" + rpcp->get_description().get_hash();
                    lua->set_usertype(type_name, type_reg);
                    while (device_protocol.first->waitReceived(CommunicationDevice::Duration{0}, 1)) {
                        //ignore leftover data in the receive buffer
                    }
                    rpcp->clear();
                } else {
                    //TODO: other protocols
                    throw std::runtime_error("invalid protocol: " + device_protocol.second->type.toStdString());
                }
            }
            (*lua)["run"](device_list);
        }
        reset_lua_state();
    } catch (const sol::error &e) {
        set_error(e);
        reset_lua_state();
        throw;
    }
}
