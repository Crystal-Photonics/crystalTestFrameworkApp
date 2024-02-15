/// \cond HIDDEN_SYMBOLS

#include "lua_functions.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "console.h"
#include "scriptengine.h"
//#include "util.h"
#include "vc.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <cmath>
#if defined(Q_OS_WIN)
#include <lmcons.h>
#include <windows.h>
#endif

/// \endcond

#if 1
/** \defgroup convenience Convenience functions
 *  A Collection of built in convenience functions
 *
 * \author Tobias Rieger (tr@crystal-photonics.com)
 * \author Arne Krüger (ak@crystal-photonics.com)
 * \brief  Lua interface
 * \par Describes the custom functions available to the LUA scripts.
 * \{
 */
#endif

/// \cond HIDDEN_SYMBOLS

std::vector<unsigned int> measure_noise_level_distribute_tresholds(const unsigned int length, const double min_val, const double max_val) {
    std::vector<unsigned int> retval;
    double range = max_val - min_val;
    for (unsigned int i = 0; i < length; i++) {
        unsigned int val = std::round(i * range / length) + min_val;
        retval.push_back(val);
    }
    return retval;
}
/// \endcond

/*! \fn double measure_noise_level_czt(device rpc_device, int dacs_quantity, int max_possible_dac_value)
\brief Calculates the noise level of an CZT-Detector system with thresholded radioactivity counters.
\param rpc_device The communication instance of the CZT-Detector.
\param dacs_quantity Number of thresholds which is also equal to the number of counter results.
\param max_possible_dac_value Max digit of the DAC which controlls the thresholds. For 12 Bit this value equals 4095.
\return The lowest DAC threshold value which matches with the noise level definition.

\details This function modifies DAC thresholds in order to find the
   lowest NoiseLevel which matches with:<br> \f$ \int_{NoiseLevel}^\infty
   spectrum(energy) < LimitCPS \f$ <br> where LimitCPS is defined by
   configuration and is set by default to 5 CPS. <br> <br> Because this
   functions iterates through the range of the spectrum to the find the noise
   level it is necessary to have write access to the DAC thresholds and read
   access to the count values at the DAC thresholds. This is done by two call
   back functions which have to be implemented by the user into the lua script:
   <br><br>

\par function callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts
\code{.lua}
function callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts(
                    rpc_device,
                    thresholds,
                    integration_time_s,
                    count_limit_for_accumulation_abort)
    \endcode
\brief  Shall modify the DAC thresholds and accumulate the counts of the counters with the new thresholds.
\param rpc_device The communication instance of the CZT-Detector.
\param thresholds Table with the length of \c dacs_quantity containing the thresholds which have to be set by this function.
\param integration_time_s Time in seconds to acquire the counts of the thresholded radioactivity counters.
\param count_limit_for_accumulation_abort If not equal to zero the function can abort the count acquisition if one counter reaches
count_limit_for_accumulation_abort.
\return Table of acquired counts in order of the table \c thresholds.
\par function callback_measure_noise_level_restore_dac_thresholds_to_normal_mode
\code{.lua}
function callback_measure_noise_level_restore_dac_thresholds_to_normal_mode(rpc_device)
\endcode
\brief Modifying thresholds often implies that a special "overwrite with
   custom thresholds"-mode is required. This function allows the user to leave
   this mode.
\param rpc_device The communication instance of the CZT-Detector.
\return nothing.

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
                                                --of 10 seconds and add the results sleep_ms(1000) local count_values = rpc_device:get_counts_raw(1)
                count_values_akku = table_add_table(count_values_akku,count_values)
                if (table_max(count_values_akku) > count_limit_for_accumulation_abort) and (count_limit_for_accumulation_abort ~= 0) then
                    break
                end
            end

            local counts_cps = table_mul_constant(count_values_akku,1/integration_time_s)
            print("measured counts[cps]:", counts_cps)
            return count_values_akku end \endcode
\code{.lua}
function callback_measure_noise_level_restore_dac_thresholds_to_normal_mode(rpc_device)
--eg:
rpc_device:thresholds_set_custom_values(0, 0,0,0,0) --disable overwriting
--thresholds with custom values end \endcode

*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double measure_noise_level_czt(device rpc_device, int dacs_quantity, int max_possible_dac_value);
#endif

/// \cond HIDDEN_SYMBOLS
double measure_noise_level_czt(sol::state &lua, sol::table rpc_device, const unsigned int dacs_quantity, const unsigned int max_possible_dac_value) {
    const unsigned int THRESHOLD_NOISE_LEVEL_CPS = 5;
    const unsigned int INTEGRATION_TIME_SEC = 1;
    const unsigned int INTEGRATION_TIME_HIGH_DEF_SEC = 10;
    double noise_level_result = 100000000;
    // TODO: test if dacs_quantity > 1

    std::vector<unsigned int> dac_thresholds = measure_noise_level_distribute_tresholds(dacs_quantity, 0, max_possible_dac_value);

    for (unsigned int i = 0; i < max_possible_dac_value; i++) {
        sol::table dac_thresholds_lua_table = lua.create_table_with();
        for (auto j : dac_thresholds) {
            dac_thresholds_lua_table.add(j);
        }
        sol::table counts = sol_call<sol::table>(lua, "callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts", rpc_device,
                                                 dac_thresholds_lua_table, INTEGRATION_TIME_SEC, 0);

#if 0
        sol::protected_function callback = lua["callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts"];
        sol::protected_function_result result = callback(rpc_device, dac_thresholds_lua_table, INTEGRATION_TIME_SEC, 0);

        if (result.valid() == false) {
            qDebug() << "result.get_type:" << (int)result.get_type();
            qDebug() << "result.status:" << (int)result.status();
            throw std::runtime_error(
                "callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts() returned an invalid value. (e.g nil) Most likely this function has "
                "crashed.");
        }
        sol::table counts = result;
#endif
        // print counts here
        for (auto &j : counts) {
            double val = std::abs(j.second.as<double>());
            qDebug() << val;
        }
        double window_start = 0;
        double window_end = 0;

        for (int j = counts.size() - 1; j >= 0; j--) {
            if (counts[j + 1].get<double>() > THRESHOLD_NOISE_LEVEL_CPS) {
                window_start = dac_thresholds[j];
                window_end = window_start + (dac_thresholds[1] - dac_thresholds[0]);
                qDebug() << "window_start" << window_start;
                qDebug() << "window_end" << window_end;
                break;
            }
        }
        dac_thresholds = measure_noise_level_distribute_tresholds(dacs_quantity, window_start, window_end);

        qDebug() << "new threshold:"; // << dac_thresholds;

        if (dac_thresholds[0] == dac_thresholds[dacs_quantity - 1]) {
            noise_level_result = dac_thresholds[0];
            break;
        }
    }

    // feinabstung und Plausibilitätsprüfung
    for (unsigned int i = 0; i < max_possible_dac_value; i++) {
        sol::table dac_thresholds_lua_table = lua.create_table_with();
        for (unsigned int i = 0; i < dacs_quantity; i++) {
            dac_thresholds_lua_table.add(uint(round(noise_level_result)));
        }
        // print("DAC:",rauschkante)
        sol::table counts =
            sol_call<sol::table>(lua, "callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts", rpc_device, dac_thresholds_lua_table,
                                 INTEGRATION_TIME_HIGH_DEF_SEC, INTEGRATION_TIME_HIGH_DEF_SEC * THRESHOLD_NOISE_LEVEL_CPS);
#if 0
         sol::protected_function callback = lua["callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts"];
        sol::protected_function_result result =
            callback(rpc_device, dac_thresholds_lua_table, INTEGRATION_TIME_HIGH_DEF_SEC, INTEGRATION_TIME_HIGH_DEF_SEC * THRESHOLD_NOISE_LEVEL_CPS);


        if (result.valid() == false) {
            qDebug() << "result.get_type:" << (int)result.get_type();
            qDebug() << "result.status:" << (int)result.status();
            throw std::runtime_error(
                "callback_measure_noise_level_set_dac_thresholds_and_get_raw_counts() returned an invalid value. (e.g nil) Most likely this function has "
                "crashed.");
        }

        sol::table counts = result;
#endif
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
    sol_call(lua, "callback_measure_noise_level_restore_dac_thresholds_to_normal_mode", rpc_device);

#if 0
    sol::protected_function callback = lua["callback_measure_noise_level_restore_dac_thresholds_to_normal_mode"];
    sol::protected_function_result result = callback(rpc_device);
    if (result.valid() == false) {
        qDebug() << "result.get_type:" << (int)result.get_type();
        qDebug() << "result.status:" << (int)result.status();
        throw std::runtime_error(
            "callback_measure_noise_level_restore_dac_thresholds_to_normal_mode() returned an invalid value.Most likely this function has "
            "crashed.");
    }
#endif

    // lua["callback_measure_noise_level_restore_dac_thresholds_to_normal_mode"](rpc_device);
    return noise_level_result;
}
/// \endcond

/*! \fn string show_file_save_dialog(string title, string path, string_table filter);
 * \brief Shows a filesave dialog
 * \param title string value
which is shown as the title of the window.
\param path preselected path of the dialog
\param filter a table of strings with the file filters the user can select

\return the selected filename
\sa show_file_open_dialog()

\details
The call is blocking, meaning the script pauses until the user clicks ok.

\par example:
\code{.lua}
result = show_file_save_dialog("Save copy",".",{"Images (*.png *.xpm *.jpg)", "Text files (*.txt)"})
--file save dialog appears and waits till user
selects file print(result) -- will print selected filename \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string show_file_save_dialog(string title, string path, string_table filter);
#endif
#if 1
/// \cond HIDDEN_SYMBOLS
std::string show_file_save_dialog(const std::string &title, const std::string &path, sol::table filters) {
    QStringList sl;
    for (auto &i : filters) {
        sl.append(QString::fromStdString(i.second.as<std::string>()));
    }

    QString filter = sl.join(";");
    QString result = Utility::promised_thread_call(MainWindow::mw, [&title, &path, &filter]() {
        return QFileDialog::getSaveFileName(MainWindow::mw, QString::fromStdString(title), QString::fromStdString(path), filter);
    });

    return result.toStdString();
}
#endif
/// \endcond

/*! \fn string show_file_open_dialog(string title, string path, string_table filter);
 * \brief Shows a file open dialog
 * \param title string value
which is shown as the title of the window.
\param path preselected path of the dialog
\param filter a table of strings with the file filters the user can select

\return the selected filename
\sa show_file_save_dialog()

\details
The call is blocking, meaning the script pauses until the user selects a file or closes the dialogue.

\par example:
\code{.lua}
result = show_file_open_dialog("Open File",".",{"Images (*.png *.xpm *.jpg)", "Text files (*.txt)"})
--file open dialog appears and waits till user selects file
print(result) -- will print selected filename \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for documentation purpose
string show_file_open_dialog(string title, string path, string_table filter);
#endif
#if 1
/// \cond HIDDEN_SYMBOLS
std::string show_file_open_dialog(const std::string &title, const std::string &path, sol::table filters) {
    QStringList sl;
    for (auto &i : filters) {
        sl.append(QString::fromStdString(i.second.as<std::string>()));
    }

    QString filter = sl.join(";");
    QString result = Utility::promised_thread_call(MainWindow::mw, [&title, &path, &filter]() {
        return QFileDialog::getOpenFileName(MainWindow::mw, QString::fromStdString(title), QString::fromStdString(path), filter);
    });

    return result.toStdString();
}
#endif
/// \endcond

/*! \fn string show_question(string title, string message, string_table button_table);
\brief Shows a dialog window with different buttons to click.
\param title             string value which is shown as the title of the window.
\param message           string value which is shown as the message text of the window.
\param button_table      a table of strings defining which buttons will appear on the dialog.

\return the name of the clicked button as a string value.
\sa show_info()
\sa show_warning()
\details
The call is blocking, meaning the script pauses until the user clicks ok.

Following button strings are allowed:
 - "OK"
 - "Open"
 - "Save"
 - "Cancel"
 - "Close"
 - "Discard"
 - "Apply"
 - "Reset"
 - "Restore"
 - "Help"
 - "Save All"
 - "Yes"
 - "Yes to All"
 - "No"
 - "No to All"
 - "Abort"
 - "Retry"
 - "Ignore"

\par example:
\code{.lua}
result = show_question("hello","this is a hello world message.",{"yes", "no", "Cancel})
--script pauses until user clicks a button.
print(result) --will print either "Yes", "No" or "Cancel"\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string show_question(string title, string message, string_table button_table);
#endif
#if 1
/// \cond HIDDEN_SYMBOLS
std::string show_question(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message, sol::table button_table) {
    QMessageBox::StandardButtons buttons{};
#if 1
    for (auto &i : button_table) {
        QString button_name{QString::fromStdString(i.second.as<std::string>())};
        button_name = button_name.toLower();
        if (button_name == "ok") {
            buttons |= QMessageBox::Ok;
        } else if (button_name == "open") {
            buttons |= QMessageBox::Open;
        } else if (button_name == "save") {
            buttons |= QMessageBox::Save;
        } else if (button_name == "cancel") {
            buttons |= QMessageBox::Cancel;
        } else if (button_name == "close") {
            buttons |= QMessageBox::Close;
        } else if (button_name == "discard") {
            buttons |= QMessageBox::Discard;
        } else if (button_name == "apply") {
            buttons |= QMessageBox::Apply;
        } else if (button_name == "reset") {
            buttons |= QMessageBox::Reset;
        } else if (button_name == "restore defaults") {
            buttons |= QMessageBox::RestoreDefaults;
        } else if (button_name == "help") {
            buttons |= QMessageBox::Help;
        } else if (button_name == "save all") {
            buttons |= QMessageBox::SaveAll;
        } else if (button_name == "yes") {
            buttons |= QMessageBox::Yes;
        } else if (button_name == "yes to all") {
            buttons |= QMessageBox::YesToAll;
        } else if (button_name == "no") {
            buttons |= QMessageBox::No;
        } else if (button_name == "no to all") {
            buttons |= QMessageBox::NoToAll;
        } else if (button_name == "abort") {
            buttons |= QMessageBox::Abort;
        } else if (button_name == "retry") {
            buttons |= QMessageBox::Retry;
        } else if (button_name == "ignore") {
            buttons |= QMessageBox::Ignore;
        } else if (button_name == "nobutton") {
            buttons |= QMessageBox::NoButton;
        }
    }
#endif
    int result = 0;
    result = Utility::promised_thread_call(MainWindow::mw, [&path, &title, &message, buttons]() {
        return QMessageBox::question(MainWindow::mw, QString::fromStdString(title.value_or("nil")) + " from " + path,
                                     QString::fromStdString(message.value_or("nil")), buttons);
    });
    switch (result) {
        case QMessageBox::Ok:
            return "OK";
        case QMessageBox::Open:
            return "Open";
        case QMessageBox::Save:
            return "Save";
        case QMessageBox::Cancel:
            return "Cancel";
        case QMessageBox::Close:
            return "Close";
        case QMessageBox::Discard:
            return "Discard";
        case QMessageBox::Apply:
            return "Apply";
        case QMessageBox::Reset:
            return "Reset";
        case QMessageBox::RestoreDefaults:
            return "Restore Defaults";
        case QMessageBox::Help:
            return "Help";
        case QMessageBox::SaveAll:
            return "Save All";
        case QMessageBox::Yes:
            return "Yes";
        case QMessageBox::YesToAll:
            return "Yes to All";
        case QMessageBox::No:
            return "No";
        case QMessageBox::NoToAll:
            return "No to All";
        case QMessageBox::Abort:
            return "Abort";
        case QMessageBox::Retry:
            return "Retry";
        case QMessageBox::Ignore:
            return "Ignore";
    }
    return "";
}
#endif
/// \endcond

/*! \fn show_info(string title, string message);
\brief Shows a message window with an info icon.
\param title             string value which is shown as the title of the window.
\param message           string value which is shown as the message text of the
window.

\sa show_question()
\sa show_warning()

\details
The call is blocking, meaning the script pauses until the user clicks ok.

\par example:
\code{.lua}
    show_warning("hello","this is a hello world message.") --script pauses until
user clicks ok. \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
show_info(string title, string message);
#endif

/// \cond HIDDEN_SYMBOLS
void show_info(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
    Utility::promised_thread_call(MainWindow::mw, [&path, &title, &message]() {
        QMessageBox::information(MainWindow::mw, QString::fromStdString(title.value_or("nil")) + " from " + path,
                                 QString::fromStdString(message.value_or("nil")));
    });
}
/// \endcond

/*! \fn show_warning(string title, string message);
\brief Shows a message window with a warning icon.
\param title             string value which is shown as the title of the window.
\param message           string value which is shown as the message text of the
window.

\sa show_question()
\sa show_info()

\details
The call is blocking, meaning the script pauses until the user clicks ok.

\par example:
\code{.lua}
    show_warning("hello","this is a hello world message.") --script pauses until
user clicks ok. \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
show_warning(string title, string message);
#endif

/// \cond HIDDEN_SYMBOLS
void show_warning(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
    Utility::promised_thread_call(MainWindow::mw, [&path, &title, &message]() {
        QMessageBox::warning(MainWindow::mw, QString::fromStdString(title.value_or("nil")) + " from " + path, QString::fromStdString(message.value_or("nil")));
    });
}
/// \endcond

/*! \fn print(argument);
\brief Prints the string value of \c argument to the console.
\param argument             Input value to be printed. Prints all types except
for userdefined ones which come the scriptengine. eg. rpc_devices or Ui
elements.


\details
\par Differences to the standard Lua world:
  - Besides the normal way you can concatenate arguments using a "," comma.
Using multiple arguments.

\par example:
\code{.lua}
    local table_array = {1,2,3,4,5}
    local table_struct = {["A"]=-5, ["B"]=-4,["C"]=-3,["D"]=-2,["E"]=-1}
    local int_val = 536

    print("Hello world")                 -- prints "Hello world"
    print(1.1487)                        -- prints 1.148700
    print(table_array)                   --{1,2,3,4,5}
    print(table_struct)                  -- {["B"]=-4, ["C"]=-3, ["D"]=-2,
["E"]=-1} print("outputstruct: ",table_struct) -- "outputstruct: "{["D"]=-2,
["C"]=-3,
                                         --                ["F"]=0, ["E"]=-1}
    print("output int as string: "..int_val) --"output int as string: 536"
    print("output int: ",int_val)        --"output int: "536
    print(non_declared_variable)         --nil
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
print(argument);
#endif

/// \cond HIDDEN_SYMBOLS
void print(QPlainTextEdit *console, const sol::variadic_args &args) {
    std::string text;
    for (auto &object : args) {
        text += ScriptEngine::to_string(object);
    }
    Utility::thread_call(MainWindow::mw, [console = console, text = std::move(text)] {
        qDebug() << QString::fromStdString(text);
        Console_handle::script(console) << text;
    });
}
/// \endcond

/*! \fn sleep_ms(int timeout_ms);
\brief Pauses the script for \c timeout_ms milliseconds.
\param timeout_ms          Positive integer input value.

,\details    \par example:
\code{.lua}
    local timeout_ms = 100
    sleep_ms(timeout_ms)  -- waits for 100 milliseconds
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
sleep_ms(int timeout_ms);
#endif

/// \cond HIDDEN_SYMBOLS
void sleep_ms(ScriptEngine *scriptengine, const unsigned int duration_ms, const unsigned int starttime_ms) {
    scriptengine->await_timeout(std::chrono::milliseconds{duration_ms}, std::chrono::milliseconds{starttime_ms});
#if 0
    QEventLoop event_loop;
    static const auto secret_exit_code = -0xF42F;
    QTimer::singleShot(timeout_ms, [&event_loop] { event_loop.exit(secret_exit_code); });
    auto exit_value = event_loop.exec();
    if (exit_value != secret_exit_code) {
        throw sol::error("Interrupted");
    }
#endif
}
/// \endcond

/*! \fn double current_date_time_ms();
\brief Returns the current Milliseconds since epoch (1970-01-01T00:00:00.000).

\details    \par example:
\code{.lua}
    local value = current_date_time_ms()
    print(value)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double current_date_time_ms();
#endif

/// \cond HIDDEN_SYMBOLS
double current_date_time_ms() {
    return QDateTime::currentMSecsSinceEpoch();
}
/// \endcond

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
// this block is just for ducumentation purpose
double round(double value, int precision);
#endif

///
/// \cond HIDDEN_SYMBOLS
double round_double(const double value, const unsigned int precision) {
    double faktor = pow(10, precision);
    double retval = value;
    retval *= faktor;
    retval = std::round(retval);
    return retval / faktor;
}
/// \endcond

/*! \fn double table_crc16(number_table input_values);
\brief Calculated the CRC16 checksum of \c input_values assuming these values
are an array of uint8_t \param input_values                 Input table of int
values < 255.

\return                     The crc16 value of \c input_values.

\details    \par example:
\code{.lua}
    local input_values = {20, 40, 2, 30}
    local retval = table_crc16(input_values)
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
int table_crc16(number_table input_values);
#endif
/// \cond HIDDEN_SYMBOLS

uint16_t table_crc16(QPlainTextEdit *console, sol::table input_values) {
    uint8_t x = 0;
    uint16_t crc = 0xFFFF;

    for (auto &i : input_values) {
        if (i.second.as<double>() > 255) {
            const auto &message = QObject::tr("table_crc16: found field value > 255");
            Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
            throw sol::error("Unsupported table field type");
        }
        uint8_t item = i.second.as<double>();
        x = crc >> 8 ^ item;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }

    return crc;
}
/// \endcond

/*! \fn double table_sum(number_table input_values);
\brief Returns the sum of the table \c input_values
\param input_values                 Input table of int or double values.

\return                     The sum value of \c input_values.

\details
\par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local retval = table_sum(input_values)  -- retval is -28.0
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_sum(number_table input_values);
#endif
/// \cond HIDDEN_SYMBOLS

double table_sum(sol::table input_values) {
    double retval = 0;
    for (auto &i : input_values) {
        retval += i.second.as<double>();
    }
    return retval;
}
/// \endcond

/*! \fn bool table_contains_string(string_table input_values, string search_text);
\brief Returns whether \c input_values contains \c search_text
\param input_values                 Input table of string values.
\param search_text                  String to be searched for.

\return True if \c input_values contains \c search_text

\details
\par example:
\code{.lua}
    local input_values = {"hello", "world", "foo", "bar"}
    local retval = table_find_string(input_values, "hello")  -- retval is true
    print(retval)
    retval = table_find_string(input_values, "test")  -- retval is false
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
bool table_contains_string(string_table input_values, string search_text);
#endif

/// \cond HIDDEN_SYMBOLS
bool table_contains_string(sol::table input_table, std::string search_text) {
    return table_find_string(input_table, search_text) > 0;
}
/// \endcond

/*! \fn uint table_find_string(string_table input_values, string search_text);
\brief Returns the index of \c search_text in table \c input_values
\param input_values                 Input table of string values.
\param search_text                  String to be searched for.

\return                     The position of \c search_text in \c input_values.
If \c search_text is not found the value 0 is returned.

\details    \par example:
\code{.lua}
    local input_values = {"hello", "world", "foo", "bar"}
    local retval = table_find_string(input_values, "hello")  -- retval is 1
    print(retval)
    retval = table_find_string(input_values, "test")  -- retval is 0
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
uint table_find_string(string_table input_values, string search_text);
#endif

/// \cond HIDDEN_SYMBOLS
uint table_find_string(sol::table input_table, std::string search_text) {
    uint retval = 0;
    for (auto &i : input_table) {
        retval++;
        if (i.second.as<std::string>() == search_text) {
            return retval;
        }
    }
    return 0;
}

static void lua_object_to_string(QPlainTextEdit *console, sol::object &obj, QString &v, QString &t) {
    v = "";
    t = "";
    if (obj.get_type() == sol::type::number) {
        v = QString::number(obj.as<double>());
        t = "n";
    } else if (obj.get_type() == sol::type::boolean) {
        if (obj.as<bool>()) {
            v = "1";
        } else {
            v = "0";
        }
        t = "b";
    } else if (obj.get_type() == sol::type::nil) {
        v = "";
        t = "l";
    } else if (obj.get_type() == sol::type::string) {
        v = QString::fromStdString(obj.as<std::string>());
        v = v.replace("\"", "\\\"");
        // v = "\"" + v + "\"";
        t = "s";
    } else {
        const auto &message = QObject::tr("Failed to save table to file. Unsupported table field type.");
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("Unsupported table field type");
    }
}

static void table_to_json_object(QPlainTextEdit *console, QJsonArray &jarray, const sol::table &input_table) {
    for (auto &i : input_table) {
        QJsonObject jobj;
        QString f;
        QString t;
        lua_object_to_string(console, i.first, f, t);
        jobj["i"] = f;
        jobj["ti"] = t;
        if (i.second.get_type() == sol::type::table) {
            QJsonArray jarray;
            table_to_json_object(console, jarray, i.second.as<sol::table>());
            jobj["v"] = jarray;
        } else {
            QString t;
            QString s;
            lua_object_to_string(console, i.second, s, t);
            jobj["v"] = s;
            jobj["tv"] = t;
        }
        jarray.append(jobj);
    }
}
/// \endcond

/*! \fn table_save_to_file(string file_name, table input_table, bool over_write_file);
\brief Writes an arbitrary lua table to file.
\param file_name The filename to write the file to.
\param input_table The table to be saved.
\param over_write_file whether overwrite a potentially existing file(true) or not(false)

\sa propose_unique_filename_by_datetime(text dir_path, text prefix, text suffix)
\sa table_load_from_file()

\details
The function writes the lua table into a \glos{json} file which is a normal text
file. The table is written into the \glos{json} array "table" where its elements
contain the text elements: \li \c i: The index of the Lua table element. \li \c
ti: The index type of Lua table. \li \c v: The value of Lua table element. \li
\c tv: The value type of Lua table element.

The type of the index an the value is coded as follows:
\li \c n: Number
\li \c b: Boolean
\li \c l: Nil
\li \c s: String

\par example:
\code{.json}
{
    "table": [
        {
            "i": "1",
            "ti": "n",
            "tv": "n",
            "v": "0"
        },
        {
            "i": "2",
            "ti": "n",
            "tv": "n",
            "v": "9"
        }
    ]
}
\endcode
This example is the result of the following code snippet:

\code{.lua}
    local table_example = {}
    table_example[1] = 0
    table_example[2] = 9
    table_save_to_file("testfile.json",table_example,false)
\endcode

*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
table_save_to_file(string file_name, table input_table, bool over_write_file);
#endif
/// \cond HIDDEN_SYMBOLS

void table_save_to_file(QPlainTextEdit *console, const std::string file_name, sol::table input_table, bool over_write_file) {
    QString fn = QString::fromStdString(file_name);

    if (fn == "") {
        const auto &message = QObject::tr("Failed open file for saving table: %1").arg(fn);
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("could not open file");
        return;
    }

    if (!over_write_file && QFile::exists(fn)) {
        const auto &message = QObject::tr(
                                  "File for saving table already exists "
                                  "and must not be overwritten: %1")
                                  .arg(fn);
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("File already exists");
        return;
    }
    QFile saveFile(fn);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        const auto &message = QObject::tr("Failed open file for saving table: %1").arg(fn);
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("could not open file");
        return;
    }

    QJsonArray jarray;

    table_to_json_object(console, jarray, input_table);
    QJsonObject obj;
    obj["table"] = jarray;
    QJsonDocument saveDoc(obj);
    saveFile.write(saveDoc.toJson());
}

static sol::object sol_object_from_type_string(QPlainTextEdit *console, sol::state &lua, const QString &value_type, const QString &v) {
    if (value_type == "s") {
        return sol::make_object(lua, v.toStdString());
    } else if (value_type == "b") {
        if (v == "1") {
            return sol::make_object(lua, true);
        } else {
            return sol::make_object(lua, false);
        }
    } else if (value_type == "n") {
        bool ok = false;
        double index_d = v.toFloat(&ok);
        if (!ok) {
            const auto &message = QObject::tr(
                                      "Could not convert string value to number while loading "
                                      "file to table. Failed string: \"%1\"")
                                      .arg(v);
            Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
            throw sol::error("Conversion Error");
        }
        return sol::make_object(lua, index_d);
    } else {
        const auto &message = QObject::tr("Unknown value type in file to be loaded into table. Type: \"%1\"").arg(value_type);
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("unknown type");
    }
}

static sol::table jsonarray_to_table(QPlainTextEdit *console, sol::state &lua, const QJsonArray &jarray) {
    sol::table result = lua.create_table_with();

    for (auto jv : jarray) {
        QJsonObject obj = jv.toObject();

        QString index_type = obj["ti"].toString();

        sol::object val = [&obj, &lua, console]() -> sol::object {
            if (obj["v"].isArray()) {
                sol::table luatable = jsonarray_to_table(console, lua, obj["v"].toArray());
                return luatable;
            } else {
                QString value = obj["v"].toString();
                QString value_type = obj["tv"].toString();

                return sol_object_from_type_string(console, lua, value_type, value);
            }
        }();

        if (index_type == "s") {
            QString index = obj["i"].toString();
            result[index.toStdString()] = val;
        } else if (index_type == "b") {
        } else if (index_type == "n") {
            QString index = obj["i"].toString();
            bool ok = false;
            double index_d = index.toFloat(&ok);
            if (!ok) {
                const auto &message = QObject::tr(
                                          "Could not convert string index to number while "
                                          "loading file to table. Failed string: \"%1\"")
                                          .arg(index);
                Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
                throw sol::error("Conversion Error");
            }
            result[index_d] = val;
        } else {
            const auto &message = QObject::tr(
                                      "Unknown index type in file to be "
                                      "loaded into table. Type: \"%1\"")
                                      .arg(index_type);
            Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
            throw sol::error("unknown type");
        }
    }
    return result;
}
/// \endcond

/*! \fn table table_load_from_file(string file_name)
\brief Loads a lua table from file which where written using
table_save_to_file() \param file_name            The filename to load the table
from. \returns the lua table from file.

\sa table_save_to_file()

\par example:

\code{.lua}
    local table_example = {}
    table_example[1] = 0
    table_example[2] = 9
    table_save_to_file("testfile.json",table_example,false)
    local loaded_table = table_load_from_file("testfile.json")
    print(loaded_table)
\endcode

*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
table table_load_from_file(string file_name);
#endif
/// \cond HIDDEN_SYMBOLS
sol::table table_load_from_file(QPlainTextEdit *console, sol::state &lua, const std::string file_name) {
    QString fn = QString::fromStdString(file_name);

    QFile loadFile(fn);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        const auto &message = QObject::tr("Can not open file for reading: \"%1\"").arg(fn);
        Utility::thread_call(MainWindow::mw, [console = console, message = std::move(message)] { Console_handle::error(console) << message; });
        throw sol::error("cant open file");
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    QJsonObject obj = loadDoc.object();
    QJsonArray jarray = obj["table"].toArray();
    return jsonarray_to_table(console, lua, jarray);
}

static sol::object table_minmax_by_field(sol::state &lua, sol::table input_values, const std::string field_name, bool max) {
    QString result_string;
    double result_num = 0;
    bool result_bool = false;
    bool is_first_value = true;
    sol::type initial_type = sol::type::nil;

    if (input_values.size() == 0) {
        return sol::nil;
    }

    for (auto &i : input_values) {
        if (i.second.get_type() != sol::type::table) {
            throw sol::error{
                QString("Seems the table is not of type: \"table of table\" in field %1").arg(QString::fromStdString(i.first.as<std::string>())).toStdString()};
        }
        const sol::table &obj = i.second.as<sol::table>();

        if (is_first_value == false) {
            if (initial_type != obj[field_name].get<sol::object>().get_type()) {
                throw sol::error{QString("field type of field %1 is not consistent.").arg(QString::fromStdString(field_name)).toStdString()};
            }
        }
        if (obj[field_name].get<sol::object>().get_type() == sol::type::number) {
            if (is_first_value) {
                initial_type = obj[field_name].get<sol::object>().get_type();
                result_num = obj[field_name].get<double>();
                is_first_value = false;
            } else {
                if (max) {
                    if (result_num < obj[field_name].get<double>()) {
                        result_num = obj[field_name].get<double>();
                    }
                } else {
                    if (result_num > obj[field_name].get<double>()) {
                        result_num = obj[field_name].get<double>();
                    }
                }
            }
        } else if (obj[field_name].get<sol::object>().get_type() == sol::type::boolean) {
            if (is_first_value) {
                initial_type = obj[field_name].get<sol::object>().get_type();
                is_first_value = false;
            }
            if (max) {
                if (obj[field_name].get<bool>()) {
                    result_bool = true;
                }
            } else {
                if (obj[field_name].get<bool>() == false) {
                    result_bool = false;
                }
            }
        } else if (obj[field_name].get<sol::object>().get_type() == sol::type::string) {
            auto str = QString::fromStdString(obj[field_name].get<std::string>());
            if (is_first_value) {
                initial_type = obj[field_name].get<sol::object>().get_type();
                result_string = str;
                is_first_value = false;
            } else {
                if (max) {
                    if (result_string < str) {
                        result_string = str;
                    }
                } else {
                    if (result_string > str) {
                        result_string = str;
                    }
                }
            }
        } else {
            throw sol::error{QString("Only the types string, number and boolean are allowed.").toStdString()};
        }
    }
    if (initial_type == sol::type::string) {
        return sol::make_object(lua, result_string.toStdString());
    } else if (initial_type == sol::type::number) {
        return sol::make_object(lua, result_num);
    } else if (initial_type == sol::type::boolean) {
        return sol::make_object(lua, result_bool);
    } else {
        return sol::nil;
    }
}
/// \endcond

/*! \fn variant table_max_by_field(table input_values, string field_name);
\brief Returns the max value of the table \c input_values with the field \c
field_name. \param input_values                 Input table of int, double,
string or bool values. If it is a string value the greater than / smaller than
decision is based on alphabetical order. \param field_name                 The
name of the field which will be compared.

\return                     The mean value of \c input_values.

\details    \par example:
\code{.lua}
    local tabelle_num = {{a=5, b = 3, c = 1},{a=3, b = 8, c = 1}}
    local tabelle_str = {{a="foo", b = 3, c = 1},{a="bar", b = 8, c =
1},{a="xylophon", b = 8, c = 1}}

    print(table_min_by_field(tabelle_num,"a")) --prints 3
    print(table_max_by_field(tabelle_str,"a")) --prints xylophon
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
variant table_max_by_field(table input_values, string field_name);
#endif
/// \cond HIDDEN_SYMBOLS

sol::object table_max_by_field(sol::state &lua, sol::table input_values, const std::string field_name) {
    return table_minmax_by_field(lua, input_values, field_name, true);
}

/// \endcond

/*! \fn variant table_min_by_field(table input_values, string field_name);
\brief Returns the min value of the table \c input_values with the field \c
field_name. \param input_values                 Input table of int, double,
string or bool values. If it is a string value the greater than / smaller than
decision is based on alphabetical order. \param field_name                   The
name of the field which will be compared.

\return                     The mean value of \c input_values.

\par example:
\code{.lua}
    local tabelle_num = {{a=5, b = 3, c = 1},{a=3, b = 8, c = 1}}
    local tabelle_str = {{a="foo", b = 3, c = 1},{a="bar", b = 8, c =
1},{a="xylophon", b = 8, c = 1}}

    print(table_min_by_field(tabelle_num,"a")) --prints 3
    print(table_max_by_field(tabelle_str,"a")) --prints xylophon
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
variant table_min_by_field(table input_values, string field_name);
#endif
/// \cond HIDDEN_SYMBOLS

sol::object table_min_by_field(sol::state &lua, sol::table input_values, const std::string field_name) {
    return table_minmax_by_field(lua, input_values, field_name, false);
}

/// \endcond
///
/*! \fn double table_mean(number_table input_values);
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
// this block is just for ducumentation purpose
double table_mean(number_table input_values);
#endif
/// \cond HIDDEN_SYMBOLS

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
}

/// \endcond

///
/*! \fn double table_variance(table input_values);
\brief Returns the variance value of the table \c input_values. Using the
formula s = (1/N)*SUM((x_i - avg )^2) \param input_values                 Input
table of int or double values.

\return                     The variance value of \c input_values.

\details    \par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local retval = table_variance(input_values)  -- retval is 677
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_variance(table input_values);
#endif
/// \cond HIDDEN_SYMBOLS

double table_variance(sol::table input_values) {
    double retval = 0;
    int count = 0;
    const double mean = table_mean(input_values);
    for (auto &i : input_values) {
        double diff = i.second.as<double>() - mean;
        retval += diff * diff;
        count += 1;
    }
    if (count) {
        retval /= count;
    }
    return retval;
}

/// \endcond

///
/*! \fn double table_standard_deviation(table input_values);
\brief Returns the standard deviation value of the table \c input_values. Using
the formula s = sqrt(table_variance) \param input_values                 Input
table of int or double values.

\return                     The standard deviation value of \c input_values.

\details    \par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local retval = table_standard_deviation(input_values)  -- retval is 26.01..
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_standard_deviation(table input_values);
#endif
/// \cond HIDDEN_SYMBOLS

double table_standard_deviation(sol::table input_values) {
    const double variance = table_variance(input_values);
    const double retval = sqrt(variance);
    return retval;
}

/// \endcond

/*! \fn number_table table_set_constant(number_table input_values, number constant);
\brief Returns a table with the length of \c input_values initialized with \c constant.
\param input_values Input table of \c int or \c double values.
\param constant Int or double value. The value the array is initialized with.
\return A table with the length of \c input_values initialized with \c constant.
\details
\par example:
\code{.lua}
    local input_values = {-20, -40, 2, 30}
    local constant = {2.5}
    local retval = table_set_constant(input_values,constant)  -- retval is {2.5, 2.5,
                                                              --            2.5, 2.5}
    print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_set_constant(number_table input_values, number constant);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_set_constant(sol::state &lua, sol::table input_values, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        retval.add(constant);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_create_constant(int size, double constant);
    \brief Creates a table with \c size elements initialized with \c constant.
    \param size                 Positive integer. The size of the array to be
   created. \param constant              Int or double value. The value the
   array is initialized with.

    \return                     A table with \c size elements initialized with
   \c constant.

    \details    \par example:
    \code{.lua}
        local size = 5
        local constant = {2.5}
        local retval = table_create_constant(size,constant)  -- retval is
   {2.5, 2.5,
                                                       --          2.5, 2.5, 2.5}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_create_constant(int size, double constant);
#endif
/// \cond HIDDEN_SYMBOLS
sol::table table_create_constant(sol::state &lua, const unsigned int size, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= size; i++) {
        retval.add(constant);
    }
    return retval;
}
/// \endcond
///

/*! \fn number_table table_add_table_at(number_table input_values_a,
   number_table input_values_b, int at); \brief Performs a vector addition of \c
   input_values_a[i+at-1] + \c input_values_b[i] for each i while input_values_a
   and input_values_b may have a different legth. \param input_values_a Input
   table of int or double values. \param input_values_b       Input table of int
   or double values. The summands the table \c input_values_a is added with.
    \param at                   Position offset between input_values_a and
   input_values_b.

    \return                     A table of the differences of \c
   input_values_a[i+at-1] + \c input_values_b[i] for each i.

    \details    \par example:
    \code{.lua}
        local input_values_a = {1, 2, 3, 4}
        local input_values_b = {1, 2}
        local retval_1 = table_add_table_at(input_values_a,input_values_b, 1)
            -- retval_1 is {2, 4, 3, 4}
        print(retval_1)

        local retval_3 = table_add_table_at(input_values_a,input_values_b, 3)
            -- retval_3 is {1, 2, 4, 6}
        print(retval_3)

        local retval_4 = table_add_table_at(input_values_a,input_values_b, 4)
            -- retval_4 is {1, 2, 3, 5, 2}
        print(retval_4)

        local retval_6 = table_add_table_at(input_values_a,input_values_b, 6)
            -- retval_6 is {1, 2, 3, 4, 0, 1, 2}
        print(retval_6)

    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_add_table_at(number_table input_values_a, number_table input_values_b, int at);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_add_table_at(sol::state &lua, sol::table input_values_a, sol::table input_values_b, unsigned int at) {
    // adds a table values input_values_a at a given position at to input_values_b
    // missing values are assumed to be 0
    // does not modify original tables but returns a copy with the sum
    if (at < 1) {
        throw std::runtime_error("table_add_table_at: at index must be > 0 but is " + std::to_string(at) + ".");
    }
    if (at > 1000000) { // this was probably an underflow
        throw std::runtime_error("table_add_table_at: at index must be <= 1000000 but is " + std::to_string(at) + ". Did you accidentally cause an underflow?");
    }

    sol::table retval = lua.create_table_with();

    auto get_number_or_0 = [](const sol::table &table, std::size_t index) {
        qDebug() << "Getting index" << index << "from table" << &table;
        if (1 <= index && index <= table.size()) {
            return table[index].get<double>();
        }
        return 0.;
    };

    const std::size_t result_size = std::max(input_values_a.size(), input_values_b.size() + at - 1);
    for (size_t i = 1; i <= result_size; i++) {
        retval.add(get_number_or_0(input_values_a, i) + get_number_or_0(input_values_b, i - at + 1));
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_add_table(number_table input_values_a, number_table input_values_b);
\brief Performs a vector addition of \c input_values_a[i] + \c input_values_b[i] for each i.
\param input_values_a Input table of int or double values.
\param input_values_b Input table of int or double values. The summands the table \c input_values_a is added with.

\return A table of the differences of \c
   input_values_a[i] + \c input_values_b[i] for each i.

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_a = {-1.5, 2, 0.5, 3}

        local retval = table_add_table(input_values,input_values_a)
        --retval is {-21.5, -38, 2.5, 33}
        print(retval) \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_add_table(number_table input_values_a, number_table input_values_b);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_add_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() + input_values_b[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_add_constant(number_table input_values, number constant);
\brief Performs a vector addition of  \c input_values[i] + \c constant for each i.
\param input_values Input table of int or double values.
\param constant Int or double value. The summand the table \c input_values is added with.

\return A table of the differences of \c input_values[i] + \c constant for each i.

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local constant = {2}
        local retval = table_add_constant(input_values,constant)
        -- retval is {-18, -38, 4, 32}
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_add_constant(number_table input_values, number constant);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_add_constant(sol::state &lua, sol::table input_values, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        double sum_i = input_values[i].get<double>() + constant;
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_sub_table(number_table input_values_a, number_table input_values_b);
\brief Performs a vector subtraction of  \c input_values_a[i] - \c input_values_b[i] for each i.
\param input_values_a Input table of int or double values.
\param input_values_b Input table of int or double values. The subtrahend the table \c input_values_a is subtracted with.

\return A table of the differences of \c input_values_a[i] - \c input_values_b[i] for each i.

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_a = {-1.5, 2, 0.5, 3}
        local retval = table_sub_table(input_values_a,input_values_b)
        -- retval is {-18.5, -42, 1.5, 27}
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_sub_table(number_table input_values_a, number_table input_values_b);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_sub_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() - input_values_b[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_mul_table(number_table input_values_a, number_table input_values_b);
\brief Performs a vector multiplication of \c input_values_a[i] * \c input_values_b[i] for each i.
\param input_values_a Input table of int or double values.
\param input_values_b Input table of int or double values. The factors the table \c input_values_a is multiplied with.

\return A table of the products of \c input_values_a[i] * \c input_values_b[i] for each i.

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_a = {-1.5, 2, 0.5, 3}
        local retval = table_mul_table(input_values_a,input_values_b)
        -- retval is {30, -80 , 1, 90}
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_mul_table(number_table input_values_a, number_table input_values_b);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_mul_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() * input_values_b[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_mul_constant(number_table input_values_a, double constant);
\brief Performs a vector multiplication of  \c input_values_a[i] * \c constant for each i.
\param input_values_a Input table of int or  double values.
\param constant Int or double value. The factor the table \c input_values_a is multiplied with.

\return A table of the products of \c input_values_a[i] * \c constant.

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local constant = 3
        local retval = table_mul_constant(input_values_a,constant)
        -- retval is {-60, -120, 6, 90}
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_mul_constant(number_table input_values_a, double constant);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_mul_constant(sol::state &lua, sol::table input_values_a, double constant) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        double sum_i = input_values_a[i].get<double>() * constant;
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_div_table(number_table input_values_a, number_table input_values_b);
\brief Performs a vector division of  \c input_values_a[i] / \c input_values_b[i] for each i.
\param input_values_a Input table of int or double values. Regarded as divident of the division operation.
\param input_values_b Input table of int or double values. Regarded as divisor of the division operation.

\return A table of the quotients of \c input_values_a[i] / \c input_values_b[i].

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_b = {5, 2, 8, 10}
        local retval = table_div_table(input_values_a,input_values_b)
        -- retval is {-4, -20, 0.25, 3}
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_div_table(number_table input_values_a, number_table input_values_b);
#endif
/// \cond HIDDEN_SYMBOLS

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
}
/// \endcond

/*! \fn number_table table_round(number_table input_values, int precision);
    \brief Returns a table with rounded values of \c input_values
    \param input_values     Input table of int or double values.
    \param precision        The number of digits to round

    \return                 A table of rounded values of \c input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20.35756, -40.41188, 2.1474314, 30.247764}
        local retval_0 = table_round(input_values,2)  -- retval_0 is {-20, -40
   ,2, 30} local retval_2 = table_round(input_values,2)  -- retval_2 is {-20.36,
   -40.41,
                                                                   -- 2.15, 30.25}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_round(number_table input_values, int precision);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_round(sol::state &lua, sol::table input_values, const unsigned int precision) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        double sum_i = round_double(input_values[i].get<double>(), precision);
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_abs(number_table input_values);
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
// this block is just for ducumentation purpose
number_table table_abs(number_table input_values);
#endif
/// \cond HIDDEN_SYMBOLS

sol::table table_abs(sol::state &lua, sol::table input_values) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= input_values.size(); i++) {
        double sum_i = std::abs(input_values[i].get<double>());
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_mid(number_table input_values, int start, int length);
\brief Returns a fragment of the table \c input_values
\param input_values Input table of int or double values.
\param start Start index of the fragment of \c input_values which will be returned.
\param length Length of the fragment which will be returned.

\return A fragment of the table \c input_values.

\details
\par example:
\code{.lua}
        local input_values = {-20, -40, 2, 30,25,8,68,42}
        local retval = table_mid(input_values,2,3)  -- retval is {-40,2,30}
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_mid(number_table input_values, int start, int length);
#endif
/// \cond HIDDEN_SYMBOLS
sol::table table_mid(sol::state &lua, sol::table input_values, const unsigned int start, const unsigned int length) {
    sol::table retval = lua.create_table_with();
    for (size_t i = start; i <= start + length - 1; i++) {
        double sum_i = input_values[i].get<double>();
        retval.add(sum_i);
    }
    return retval;
}
/// \endcond

/*! \fn number_table table_range(double start, double stop, double step);
    \brief Returns a table with evenly spaced values within a given interval.
    \param start               Start of interval. The interval includes this
   value. \param stop                End of interval. The interval includes this
   value. \param step               Spacing between values

    \return

    \details    \par example:
    \code{.lua}
        local start = -2
        local stop = +2
        local step = 0.5
        local retval = table_range(start,stop,step) --{-2, -1.5, -1, -0.5, 0.0,
   0.5, 1.0, 1.5, 2.0} print(retval) retval = table_range(2,-2,-0.5) --{2, 1.5,
   1, 0.5, 0.0, -0.5, -1.0, -1.5, -2.0} print(retval) \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
number_table table_range(double start, double stop, double step);
#endif
/// \cond HIDDEN_SYMBOLS
sol::table table_range(sol::state &lua, double start, double stop, double step) {
    sol::table retval = lua.create_table_with();
    if (step > 0) {
        if (start >= stop) {
            throw std::runtime_error(QObject::tr("table_range: stop value must be greater than start "
                                                 "value if step value is greater than zero. Values are: "
                                                 "start=%1, stop=%2, step=%3")
                                         .arg(start)
                                         .arg(stop)
                                         .arg(step)
                                         .toStdString());
        }
        double value = start;
        while (value <= stop) {
            retval.add(value);
            value += step;
        }
    } else if (step < 0) {
        if (start <= stop) {
            throw std::runtime_error(QObject::tr("table_range: start value must be greater than stop "
                                                 "value if step value is less than zero. Values are: "
                                                 "start=%1, stop=%2, step=%3")
                                         .arg(start)
                                         .arg(stop)
                                         .arg(step)
                                         .toStdString());
        }
        double value = start;
        while (value >= stop) {
            retval.add(value);
            value += step;
        }
    } else {
        throw std::runtime_error(QObject::tr("table_range: step value must not be 0. Values are: "
                                             "start=%1, stop=%2, step=%3")
                                     .arg(start)
                                     .arg(stop)
                                     .arg(step)
                                     .toStdString());
    }
    return retval;
}
/// \endcond

/*! \fn table table_concat(table table1, table table2);
    \brief Returns a table joining table1 with table2.
    \param start              First table
    \param stop               Second table to be joined with table1

    \return

    \details    \par example:
    \code{.lua}
        local table1 = {1,2,3}
        local table2 = {4,5,6}
        local retval = table_concat(table1,table2) --{1,2,3,4,5,6}
        print(retval)

        table1 = {"A","B","C"}
        table2 = {"D","E","F"}
        local retval = table_concat(table1,table2) --{"A","B","C","D","E","F"}
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
table table_concat(table table1, table table1);
#endif
/// \cond HIDDEN_SYMBOLS
sol::table table_concat(sol::state &lua, sol::table table1, sol::table table2) {
    sol::table retval = lua.create_table_with();
    for (size_t i = 1; i <= table1.size(); i++) {
        auto val_1 = table1[i];
        retval.add(val_1);
    }
    for (size_t i = 1; i <= table2.size(); i++) {
        auto val = table2[i];
        retval.add(val);
    }
    return retval;
}
/// \endcond

/*! \fn bool table_equal_constant(number_table input_values_a, double input_const_val);
\brief Returns true if \c input_values_a[i] == input_const_val for all \c i.
\param input_values_a A table of double or int values.
\param input_const_val A double value to compare the table with
\return true or false

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
// this block is just for ducumentation purpose
bool table_equal_constant(number_table input_values_a, double input_const_val);
#endif
/// \cond HIDDEN_SYMBOLS
bool table_equal_constant(sol::table input_values_a, double input_const_val) {
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        if (input_values_a[i].get<double>() != input_const_val) { // TODO: fix double comparison
            return false;
        }
    }
    return true;
}
/// \endcond

/*! \fn bool table_equal_table(number_table input_values_a, number_table input_values_b);
\brief Returns true if \c input_values_a[i] == input_values_b[i] for all \c i.
\param input_values_a A table of double or int values.
\param input_values_b A table of double or int values.

\return true or false

\details
\par example:
\code{.lua}
        local input_values_a = {-20, -40, 2, 30}
        local input_values_b = {-20, -40, 2, 30}
        local input_values_c = {-20, -40, 2, 31}
        local retval = table_equal_table(input_values_a,input_values_b)  -- true
        print(retval)
        local retval = table_equal_table(input_values_a,input_values_b)  -- true
        print(retval)
        local retval = table_equal_table({"A","B","C"}, {"D","E","F"})  -- false
        print(retval)
        local retval = table_equal_table({"A","B","C"}, {"A","B","C"})  -- true
        print(retval)
\endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
bool table_equal_table(number_table input_values_a, number_table input_values_b);
#endif
/// \cond HIDDEN_SYMBOLS
bool table_equal_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b) {
    for (size_t i = 1; i <= input_values_a.size(); i++) {
        if ((input_values_a[i].get_type() == sol::type::number) && (input_values_b[i].get_type() == sol::type::number)) {
            if (input_values_a[i].get<double>() != input_values_b[i].get<double>()) { // TODO: fix double comparison
                return false;
            }
        } else if ((input_values_a[i].get_type() == sol::type::string) && (input_values_b[i].get_type() == sol::type::string)) {
            if (input_values_a[i].get<std::string>() != input_values_b[i].get<std::string>()) {
                return false;
            }
        } else {
            throw std::runtime_error(QObject::tr("table_equal_table: comparison only possible between number or "
                                                 "string values but is: A[%3]=\"%1\", B[%3]=\"%2\"")
                                         .arg(QString::fromStdString(sol::type_name(lua, input_values_a[i])))
                                         .arg(QString::fromStdString(sol::type_name(lua, input_values_b[i])))
                                         .arg(i)
                                         .toStdString());
        }
    }
    return true;
}
/// \endcond

/*! \fn double table_max(number_table input_values);
    \brief Returns the maximum value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The maximum absolute value of \c
   input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_max(input_values)  -- retval is 30
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_max(number_table input_values);
#endif
/// \cond HIDDEN_SYMBOLS
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
}
/// \endcond

/*! \fn double table_min(number_table input_values);
    \brief Returns the minimum value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The minimum absolute value of \c
   input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_min(input_values)  -- retval is -40
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_min(number_table input_values);
#endif

/// \cond HIDDEN_SYMBOLS
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
}
/// \endcond

/*! \fn double table_max_abs(number_table input_values);
    \brief Returns the maximum absolute value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The maximum absolute value of \c
   input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, -40, 2, 30}
        local retval = table_max_abs(input_values)  -- retval is 40
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_max_abs(number_table input_values);
#endif

/// \cond HIDDEN_SYMBOLS
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
}
/// \endcond

/*! \fn text propose_unique_filename_by_datetime(text dir_path, text prefix,
   text suffix) \brief Returns a filename which does not exist in the directory.
    \param dir_path                  The directory where to look for a file
    \param prefix                The prefix of the filename
    \param suffix                The suffix of the filename

    \return                the first free filename of the directory.

    \details    \par example:
        The Filename will be:
            dir/prefix_datetime_index_suffix
        where index is incremented until a free filename is found. Index is
   starting at 1. The Datetime is the datetime of now().

    \code{.lua}
        local retval =
   propose_unique_filename_by_datetime("~/test/","mytestfile",".txt")
        print(retval) --prints ~/test/mytestfile-2017_06_01-13_25_30-001.txt
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
text propose_unique_filename_by_datetime(text dir_path, text prefix, text suffix);
#endif

/// \cond HIDDEN_SYMBOLS

static QString get_unique_file_name_date_time_format() {
    return "-yyyy_MM_dd-HH_mm_ss-";
}

QDateTime decode_date_time_from_file_name(const std::string &file_name, const std::string &prefix) {
    QString str = QString::fromStdString(file_name);
    str = QFileInfo{str}.baseName();
    //  qDebug() << str;
    str = str.remove(QString::fromStdString(prefix));
    str = str.left(get_unique_file_name_date_time_format().size());
    // qDebug() << str;
    QDateTime result = QDateTime::fromString(str, get_unique_file_name_date_time_format());
    // qDebug() << result.toString();
    return result;
}

std::string propose_unique_filename_by_datetime(const std::string &dir_path, const std::string &prefix, const std::string &suffix) {
    int index = 0;
    QString dir_ = QString::fromStdString(dir_path);
    QString prefix_ = QString::fromStdString(prefix);
    QString suffix_ = QString::fromStdString(suffix);

    dir_ = append_separator_to_path(dir_);
    create_path(dir_);

    QString result;
    QString currentDateTime = QDateTime::currentDateTime().toString(get_unique_file_name_date_time_format());

    // currentDateTime = "-2017_06_01-14_50_49-";
    do {
        index++;
        result = dir_ + prefix_ + currentDateTime + QString("%1").arg(index, 3, 10, QChar('0')) + suffix_;
    } while (QFile::exists(result));
    return result.toStdString();
}
/// \endcond

/*! \fn double table_min_abs(number_table input_values);
    \brief Returns the minimum absolute value of \c input_values.
    \param input_values                A table of double or int values.

    \return                            The minimum absolute value of \c
   input_values.

    \details    \par example:
    \code{.lua}
        local input_values = {-20, 2, 30}
        local retval = table_min_abs(input_values)  -- retval is 2
        print(retval)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double table_min_abs(number_table input_values);
#endif

/// \cond HIDDEN_SYMBOLS
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
/// \endcond

/*! \fn pc_speaker_beep();
    \brief Sounds the bell, using the default volume and sound.

    \details    \par example:
    \code{.lua}
        pc_speaker_beep()
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
pc_speaker_beep();
#endif

/// \cond HIDDEN_SYMBOLS
void pc_speaker_beep() {
    QApplication::beep();
}
/// \endcond

/*! \fn run_external_tool(string execute_directory, string executable,
   string_table arguments, number timout_s) \brief runs an external tool and
   returns its output as string. \param execute_directory   The directory where
   the external tool is executed from \param executable          The path to the
   external tool. Can be a *.bat(windows), an *.exe or a *.sh(linux) file \param
   arguments           A table of argument strings. \param timout_s Timeout in
   seconds

    \return                    The output of the external tool
    \details    \par example:
    \code{.lua}
        local result = run_external_tool(".","JLink.exe
   ",{"-if","SWD","-speed","4000","-autoconnect","1","-CommanderScript","flash.jlink"},20)
        print(result)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string run_external_tool(string execute_directory, string executable, string_table arguments, number timout_s);
#endif

/// \cond HIDDEN_SYMBOLS

QString run_external_tool(const QString &script_path, const QString &execute_directory, const QString &executable, const sol::table &arguments, uint timeout) {
    QString program = search_in_search_path(script_path, executable);

    if (!QFile::exists(program)) {
        throw std::runtime_error(QObject::tr("Could not find executable at \"%1\"").arg(program).toStdString());
    }
    if (!QFile::exists(execute_directory)) {
        throw std::runtime_error(QObject::tr("Could not find directory at \"%1\"").arg(execute_directory).toStdString());
    }

    QProcessEnvironment sys_env = QProcessEnvironment::systemEnvironment();
    QString search_path = get_search_paths(script_path);
    if (sys_env.contains("path")) {
        sys_env.insert("path", search_path);
    } else if (sys_env.contains("PATH")) {
        sys_env.insert("PATH", search_path);
    }

    QStringList sl;
    for (auto &i : arguments) {
        sl.append(QString::fromStdString(i.second.as<std::string>()));
    }

    QProcess myProcess(nullptr);
    myProcess.setWorkingDirectory(execute_directory);
    myProcess.setProcessEnvironment(sys_env);
    myProcess.start(program, sl);
    myProcess.waitForStarted(timeout * 1000);
    bool proc_timeout = myProcess.waitForFinished(timeout * 1000) == false;
    QString result = QString(myProcess.readAll());
    result = result.replace("\r\n", "\n");
    if ((myProcess.exitCode() != 0) || (proc_timeout)) {
        if (proc_timeout) {
            myProcess.kill();
            throw std::runtime_error("run_external_tool(" + executable.toStdString() + ") timed out: " + QString::number(timeout).toStdString() + "s:\n " +
                                     myProcess.readAllStandardError().toStdString() + "\n " + result.toStdString());
        } else {
            throw std::runtime_error("run_external_tool(" + executable.toStdString() +
                                     ") exit with code: " + QString::number(myProcess.exitCode()).toStdString() + ":\n " +
                                     myProcess.readAllStandardError().toStdString() + "\n " + result.toStdString());
        }
    }
    return result;
}
/// \endcond

/*! \fn git_info(string path, bool allow_modified)
    \brief returns the git revision hash of a given repository
    \param path                A String containing the path to the git
   repository \param allow_modified      If false this function will throw an
   exception if the repository contains modifications which are not yet
   commited.

    \return                    a table with the git-hash, the date and the
   modified-flag of the last commit. \details    \par example: \code{.lua} git =
   git_info(".",false) print(git) -- git.hash = "1e46a0f", git.modified = true,
   git.date = "2017-05-19 14:26:36 +0200" \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string git_info(string path, bool allow_modified);
#endif

/// \cond HIDDEN_SYMBOLS

QMap<QString, QVariant> git_info(QString path, bool allow_modified, bool allow_exceptions) {
    QMap<QString, QVariant> result;
    bool modified = false;
    QString program = search_in_search_path("", "git");
    if (!QFile::exists(program)) {
        if (allow_exceptions) {
            throw std::runtime_error("Could not find git executable at \"" + program.toStdString() + "\"");
        } else {
            return result;
        }
    }
    if (!QFile::exists(path)) {
        if (allow_exceptions) {
            throw std::runtime_error("Could not find directory at \"" + QString(path).toStdString() + "\"");
        } else {
            return result;
        }
    }

    QList<QStringList> argument_list;
    argument_list.append(QStringList{"status", "-s"});
    argument_list.append(QStringList{"log", "--format=%h"});
    argument_list.append(QStringList{"log", "--format=%ci"});

    for (int i = 0; i < argument_list.count(); i++) {
        const auto &arguments = argument_list[i];
        QProcess myProcess(nullptr);
        myProcess.setWorkingDirectory(path);

        myProcess.start(program, arguments);
        myProcess.waitForStarted();
        myProcess.waitForFinished();
        auto out = QString(myProcess.readAll()).split("\n");
        if (myProcess.exitCode() != 0) {
            if (allow_exceptions) {
                throw std::runtime_error(myProcess.readAllStandardError().toStdString());
            } else {
                qDebug() << "git_info" << myProcess.readAllStandardError();

                return QMap<QString, QVariant>{};
            }
        }
        // qDebug() << out;
        switch (i) {
            case 0: {
                for (auto s : out) {
                    if (s.startsWith(" M")) {
                        modified = true;
                        break;
                    }
                }
                result.insert("modified", modified);
                if ((!allow_modified) && modified) {
                    if (allow_exceptions) {
                        throw std::runtime_error("Git Status: Git-Directory is modified.");
                    }
                }
            } break;
            case 1: {
                result.insert("hash", out[0]);
            } break;
            case 2: {
                result.insert("date", out[0]);
            } break;
            default: {
            } break; //
        }
    }
    return result;
}

sol::table git_info(sol::state &lua, std::string path, bool allow_modified) {
    auto map = git_info(QString::fromStdString(path), allow_modified, true);
    sol::table result = lua.create_table_with();

    result["modified"] = map["modified"].toBool();
    result["hash"] = "0x" + map["hash"].toString().toStdString();
    result["date"] = map["date"].toString().toStdString();
    return result;
}
/// \endcond

/*! \fn string get_framework_git_hash();
    \brief Returns the git hash of the last git commit of this test framework as
   string.

    \details    \par example:
    \code{.lua}
        print(get_framework_git_hash())  --prints eg 0x20F0467
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string get_framework_git_hash();
#endif

/// \cond HIDDEN_SYMBOLS
std::string get_framework_git_hash() {
    return "0x" + QString::number(GITHASH, 16).toUpper().toStdString();
}
/// \endcond

/*! \fn double get_framework_git_date_unix();
    \brief Returns the unix time of the last commit of this test framework as
   string.

    \details    \par example:
    \code{.lua}
        local unix_date_time = get_framework_git_date_unix() -- returns eg.
   1500044862 print(os.date("%X %x", v)) -- prints "14:26:49 07/19/17" \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
double get_framework_git_date_unix();
#endif

/// \cond HIDDEN_SYMBOLS
double get_framework_git_date_unix() {
    return GITUNIX;
}
/// \endcond

/*! \fn void mk_link(text link_pointing_to, text link_name)
    \brief Creates a file or a directory link
    \param link_pointing_to         The string of the file/directory the link points to
    \param link_name                The name for the link. On windows this is a .lnk file


    \details    \par example:
    \code{.lua}
        mk_link("file1","link_to_file.lnk")
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
void mk_link(text link_pointing_to, text link_name);
#endif

/// \cond HIDDEN_SYMBOLS
void mk_link(std::string link_pointing_to, std::string link_name) {
    QFile file_link(QString::fromStdString(link_pointing_to));
    auto link_name_q = QString::fromStdString(link_name);
#if defined(Q_OS_WIN)

    if (!link_name_q.endsWith(".lnk")) {
        link_name_q += ".lnk";
    }

#endif
    bool result = file_link.link(link_name_q);
    if (result == false) {
        throw std::runtime_error(
            QString("Can no create link. pointing to: %1 link name: %2").arg(QString::fromStdString(link_pointing_to)).arg(link_name_q).toStdString());
    }
}
/// \endcond

/*! \fn text file_link_points_to(text link_pointing_to)
    \brief returns target of a file or a directory link
    \param link_pointing_to               The name for the link. On windows this is a .lnk file
    \returns target path of a filesystem link.

    \details    \par example:
    \code{.lua}
        local target = file_link_points_to("link_to_file.lnk")
        print(target)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
text file_link_points_to(text link_pointing_to);
#endif

/// \cond HIDDEN_SYMBOLS
std::string file_link_points_to(std::string link_name) {
    auto link_name_q = QString::fromStdString(link_name);
    QFile file_link(link_name_q);

#if defined(Q_OS_WIN)
    if (!link_name_q.endsWith(".lnk")) {
        throw std::runtime_error(
            QString("The link %1 is to be resolved. But under windows file links always end with *.lnk. This is not the case with the given link.")
                .arg(link_name_q)
                .toStdString());
    }

#endif
    return file_link.symLinkTarget().toStdString();
}
/// \endcond

/*! \fn bool is_file_path_equal(text file_path_a, text file_path_b)
    \brief returns true if both paths point to the same file or directory. Works with absolute and relative paths
    \param file_path_a               First path
    \param file_path_b               Second path
    \returns true if path is equal

    \details    \par example:
    \code{.lua}
        local target = print(is_file_path_equal("15_probe_kalibrierung_in_sonde/eeprom_4A767500","15_probe_kalibrierung_in_sonde\\eeprom_4A767500\\" ))
        print(target) --returns true
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
bool is_file_path_equal(text file_path_a, text file_path_b);
#endif

/// \cond HIDDEN_SYMBOLS
bool is_file_path_equal(std::string file_path_a, std::string file_path_b) {
    auto file_info_a = QFileInfo(QString::fromStdString(file_path_a));
    auto file_info_b = QFileInfo(QString::fromStdString(file_path_b));

    auto f_a = file_info_a.absoluteFilePath();
    auto f_b = file_info_b.absoluteFilePath();
    const auto sep1 = QDir::separator();
    if ((!f_a.endsWith(sep1)) && f_b.endsWith(sep1)) {
        f_a += sep1;
    }
    if ((!f_b.endsWith(sep1)) && f_a.endsWith(sep1)) {
        f_b += sep1;
    }

    const auto sep2 = "\\";
    if ((!f_a.endsWith(sep2)) && f_b.endsWith(sep2)) {
        f_a += sep2;
    }
    if ((!f_b.endsWith(sep2)) && f_a.endsWith(sep2)) {
        f_b += sep2;
    }

    const auto sep3 = "/";
    if ((!f_a.endsWith(sep3)) && f_b.endsWith(sep3)) {
        f_a += sep3;
    }
    if ((!f_b.endsWith(sep3)) && f_a.endsWith(sep3)) {
        f_b += sep3;
    }
    return f_a == f_b;
}
/// \endcond

/*! \fn bool path_exists(text path);
    \brief returns true or false depending whether path exists or not. Works for directories, files an links. If links are inquired it returns false if the link's target is not existing
    \param path               String of the path or link
    \returns true or false

    \details    \par example:
    \code{.lua}
        local target_link = path_exists("link_to_file.lnk") -- true if the link and the target exists
        print(target_link)
        local target_dir = path_exists("link_to_directory") -- true if directory exists
        print(target_dir)
        local target_file = path_exists("link_to_file") -- true if file exists
        print(target_file)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
bool path_exists(text path);
#endif

/// \cond HIDDEN_SYMBOLS
bool path_exists(std::string path) {
    auto path_q = QString::fromStdString(path);
    QFile file_name(path_q);
    return file_name.exists();
}
/// \endcond
///
/*! \fn string get_framework_git_date_text();
    \brief Returns the date of the last commit of this test framework as string.

    \details    \par example:
    \code{.lua}
       print( get_framework_git_date_text()) -- prints eg "2017-07-14"
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string get_framework_git_date_text();
#endif

/// \cond HIDDEN_SYMBOLS
std::string get_framework_git_date_text() {
    return GITDATE;
}
/// \endcond

/*! \fn string get_os_username();
    \brief Returns the operational system's username of the current account
   beeing logged in. Works on Linux and Windows


    \details    \par example:
    \code{.lua}
        local un = get_system_username()
        print(un)
    \endcode
*/

#ifdef DOXYGEN_ONLY
// this block is just for ducumentation purpose
string get_os_username();
#endif

/// \cond HIDDEN_SYMBOLS
#if defined(Q_OS_WIN)
static QString handle_lower_upper_case_username(QString origun) {
    if (origun.size() == 2) {
        origun = origun.toUpper(); // converting e.g ak to AK
    }
    return origun;
}
#endif

std::string get_os_username() {
#if defined(Q_OS_WIN)
    WCHAR acUserName[100];
    DWORD nUserName = sizeof(acUserName);
    if (GetUserName(acUserName, &nUserName)) {
        std::wstring un(acUserName);
        auto qun = QString::fromStdString(std::string(un.begin(), un.end()));
        return handle_lower_upper_case_username(qun).toStdString();
    } else {
        throw std::runtime_error("Could not get username on Windows.");
    }
#elif defined(Q_OS_UNIX)

    QProcess process;
    process.start("whoami");
    process.waitForStarted();
    process.waitForFinished();
    auto out = QString(process.readAll());
    if (process.exitCode() != 0) {
        throw std::runtime_error(process.readAllStandardError().toStdString());
    }
    return out.toStdString();
#else
    throw std::runtime_error("Can't get username on this OS.");
#endif
}
/// \endcond
///
/** \} */ // end of group convenience

/// \cond HIDDEN_SYMBOLS
QString append_separator_to_path(QString path) {
    path = QDir::toNativeSeparators(path);
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }
    return path;
}

QString get_clean_file_path(QString filename) {
    QFileInfo fi(filename);
    if (fi.completeSuffix() == "") {
        // it is a directory
        filename = append_separator_to_path(filename);
        QFileInfo fi(filename);
        filename = fi.absoluteFilePath();
        filename = append_separator_to_path(filename);
    } else {
        filename = fi.absolutePath();
        filename = append_separator_to_path(filename);
    }
    return filename;
}

QString create_path(QString filename) {
    filename = get_clean_file_path(filename);
    QDir d(filename);
    d.mkpath(".");
    return filename;
}

static QChar get_search_path_delimiter() {
#if defined(Q_OS_WIN)
    return ';';
#elif defined(Q_OS_UNIX)
    return ':';
#endif
}

QString get_search_paths(const QString &script_path) {
    QString search_path = QSettings{}.value(Globals::search_path_key, "").toString();
    if (script_path != "") {
        search_path = search_path + get_search_path_delimiter() + get_clean_file_path(script_path);
    }
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("path")) {
        search_path += get_search_path_delimiter() + env.value("path");
    } else if (env.contains("PATH")) {
        search_path += get_search_path_delimiter() + env.value("PATH");
    }
    search_path = search_path.replace("\\", "/");
    // search_path = search_path.replace(get_path_seperator_regex(),
    // get_search_path_delimiter());
    return search_path;
}

QStringList get_lua_lib_search_paths(const QString &script_path) {
    auto search_paths = QStringList{QSettings{}.value(Globals::test_script_path_settings_key, "").toString().replace("\\", "\\\\")};
    if (script_path != "") {
        QDir script_base_path(script_path);
        script_base_path.cdUp();
        //qDebug() << script_base_path.absolutePath() << QString::fromStdString(path);
        search_paths.append(script_base_path.absolutePath());
    }

    //auto libbaths = QSettings{}.value(Globals::test_script_library_path_key, "").toString().replace("\\", "\\\\");
    auto libbaths = QSettings{}.value(Globals::test_script_library_path_key, "").toString();
    auto libbaths_list = libbaths.split(";");
    search_paths.append(libbaths_list);

    QStringList result;
    for (auto &s : search_paths) {
        result.append(get_clean_file_path(s));
    }
    return result;
}

QStringList get_lua_lib_search_paths_for_lua(const QString &script_path, const QString &lua_search_pattern) {
    //eg "/?.lua"
    auto sl = get_lua_lib_search_paths(script_path);
    QStringList result;
    for (auto &s : sl) {
        result.append(s.replace("\\", "\\\\") + lua_search_pattern);
        //result.append(s + lua_search_pattern);
    }
    //qDebug() << result;
    return result;
}
QStringList get_search_path_entries(QString search_path) {
    QStringList sl = search_path.split(get_search_path_delimiter()); // match at asasda:asfsdf but not
                                                                     // windows-like C:/asdasdas
    return sl;
}

QString search_in_search_path(const QString &script_path, const QString &file_to_be_searched) {
    QDir dir(file_to_be_searched);
    if (!dir.isRelative() && QFile::exists(file_to_be_searched)) {
        return file_to_be_searched;
    }

    QStringList sl = get_search_path_entries(get_search_paths(script_path));

    for (auto &s : sl) {
        QString path_to_test = append_separator_to_path(s);
        QDir base(QFileInfo(path_to_test).absoluteDir());
        QString result = base.absoluteFilePath(file_to_be_searched);
        //  qDebug() << result;
        if (QFile::exists(result)) {
            return result;
        }
#if defined(Q_OS_WIN)
        result = base.absoluteFilePath(file_to_be_searched + ".exe");
        // qDebug() << result;
        if (QFile::exists(result)) {
            return result;
        }
#endif
    }
    return "";
}
/// \endcond
