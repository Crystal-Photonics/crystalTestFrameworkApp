/// @cond HIDDEN_SYMBOLS

#include "lua_functions.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "console.h"
#include "scriptengine.h"
#include "util.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <cmath>

/// @endcond

/** \defgroup convenience Convenience functions
 *  A Collection of built in convenience functions
 *  \{
 */

/**
 * \file   lua_functions.cpp
 * \author Tobias Rieger (tr@crystal-photonics.com),<br> Arne Krüger (ak@crystal-photonics.com)
 * \brief  Lua interface
 * \par
 *  Describes the custom functions available to the LUA scripts.
 */

/// @cond HIDDEN_SYMBOLS

std::vector<unsigned int> measure_noise_level_distribute_tresholds(const unsigned int length, const double min_val, const double max_val) {
    std::vector<unsigned int> retval;
    double range = max_val - min_val;
    for (unsigned int i = 0; i < length; i++) {
        unsigned int val = std::round(i * range / length) + min_val;
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
                \f$ \int_{NoiseLevel}^\infty spectrum(energy) < LimitCPS \f$ <br>
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
            return count_values_akku
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

/*! \fn string show_question(string title, string message, table button_table);
\brief Shows a dialog window with different buttons to click.
\param title             string value which is shown as the title of the window.
\param message           string value which is shown as the message text of the window.
\param button_table      a table of strings defining which buttons will appear on the dialog.

\return the name of the clicked button as s string value.
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
    result = show_question("hello","this is a hello world message.",{"yes", "no", "Cancel}) --script pauses until user clicks a button.
    print(result) -- will print either "Yes", "No" or "Cancel".
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
string show_question(string title, string message, table button_table);
#endif
#if 1
/// @cond HIDDEN_SYMBOLS
std::string show_question(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message, sol::table button_table) {
    QMessageBox::StandardButtons buttons = 0;
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
};
#endif
/// @endcond

/*! \fn show_info(string title, string message);
\brief Shows a message window with an info icon.
\param title             string value which is shown as the title of the window.
\param message           string value which is shown as the message text of the window.

\sa show_question()
\sa show_warning()

\details
The call is blocking, meaning the script pauses until the user clicks ok.

\par example:
\code{.lua}
    show_warning("hello","this is a hello world message.") --script pauses until user clicks ok.
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
show_info(string title, string message);
#endif

/// @cond HIDDEN_SYMBOLS
void show_info(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
    Utility::promised_thread_call(MainWindow::mw, [&path, &title, &message]() {
        QMessageBox::information(MainWindow::mw, QString::fromStdString(title.value_or("nil")) + " from " + path,
                                 QString::fromStdString(message.value_or("nil")));
    });
};
/// @endcond

/*! \fn show_warning(string title, string message);
\brief Shows a message window with a warning icon.
\param title             string value which is shown as the title of the window.
\param message           string value which is shown as the message text of the window.

\sa show_question()
\sa show_info()

\details
The call is blocking, meaning the script pauses until the user clicks ok.

\par example:
\code{.lua}
    show_warning("hello","this is a hello world message.") --script pauses until user clicks ok.
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
show_warning(string title, string message);
#endif

/// @cond HIDDEN_SYMBOLS
void show_warning(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
    Utility::promised_thread_call(MainWindow::mw, [&path, &title, &message]() {
        QMessageBox::warning(MainWindow::mw, QString::fromStdString(title.value_or("nil")) + " from " + path, QString::fromStdString(message.value_or("nil")));
    });
};
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
        text += ScriptEngine::to_string(object);
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
void sleep_ms(ScriptEngine *scriptengine, const unsigned int timeout_ms) {
    scriptengine->timer_event_queue_run(timeout_ms);
#if 0
    QEventLoop event_loop;
    static const auto secret_exit_code = -0xF42F;
    QTimer::singleShot(timeout_ms, [&event_loop] { event_loop.exit(secret_exit_code); });
    auto exit_value = event_loop.exec();
    if (exit_value != secret_exit_code) {
        throw sol::error("Interrupted");
    }
#endif
};
/// @endcond

/*! \fn double current_date_time_ms();
\brief Returns the current Milliseconds since epoch (1970-01-01T00:00:00.000).

\details    \par example:
\code{.lua}
    local value = current_date_time_ms()
    print(value)
\endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
double current_date_time_ms();
#endif

/// @cond HIDDEN_SYMBOLS
double current_date_time_ms() {
    return QDateTime::currentMSecsSinceEpoch();
}
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

///
/// @cond HIDDEN_SYMBOLS
double round_double(const double value, const unsigned int precision) {
    double faktor = pow(10, precision);
    double retval = value;
    retval *= faktor;
    retval = std::round(retval);
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

void lua_object_to_string(QPlainTextEdit *console, sol::object &obj, QString &v, QString &t) {
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
        //v = "\"" + v + "\"";
        t = "s";
    } else {
        const auto &message = QObject::tr("Failed to save table to file. Unsupported table field type.");
        Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("Unsupported table field type");
    }
}

void table_to_json_object(QPlainTextEdit *console, QJsonArray &jarray, const sol::table &input_table) {
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

void table_save_to_file(QPlainTextEdit *console, const std::string file_name, sol::table input_table, bool over_write_file) {
    QString fn = QString::fromStdString(file_name);
    if (!over_write_file && QFile::exists(fn)) {
        const auto &message = QObject::tr("File for saving table already exists and must not be overwritten: %1").arg(fn);
        Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("File already exists");
        return;
    }
    QFile saveFile(fn);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        const auto &message = QObject::tr("Failed open file for saving table: %1").arg(fn);
        Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("could not open file");
        return;
    }

    QJsonArray jarray;

    table_to_json_object(console, jarray, input_table);
    QJsonObject obj;
    obj["table"] = jarray;
    QJsonDocument saveDoc(obj);
    saveFile.write(saveDoc.toJson());
};

sol::object sol_object_from_type_string(QPlainTextEdit *console, sol::state &lua, const QString &value_type, const QString &v) {
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
            const auto &message = QObject::tr("Could not convert string value to number while loading file to table. Failed string: \"%1\"").arg(v);
            Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
            throw sol::error("Conversion Error");
        }
        return sol::make_object(lua, index_d);
    } else {
        const auto &message = QObject::tr("Unknown value type in file to be loaded into table. Type: \"%1\"").arg(value_type);
        Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("unknown type");
    }
}

sol::table jsonarray_to_table(QPlainTextEdit *console, sol::state &lua, const QJsonArray &jarray) {
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
                const auto &message = QObject::tr("Could not convert string index to number while loading file to table. Failed string: \"%1\"").arg(index);
                Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
                throw sol::error("Conversion Error");
            }
            result[index_d] = val;
        } else {
            const auto &message = QObject::tr("Unknown index type in file to be loaded into table. Type: \"%1\"").arg(index_type);
            Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
            throw sol::error("unknown type");
        }
    }
    return result;
}

sol::table table_load_from_file(QPlainTextEdit *console, sol::state &lua, const std::string file_name) {
    QString fn = QString::fromStdString(file_name);

    QFile loadFile(fn);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        const auto &message = QObject::tr("Can not open file for reading: \"%1\"").arg(fn);
        Utility::thread_call(MainWindow::mw, [ console = console, message = std::move(message) ] { Console::error(console) << message; });
        throw sol::error("cant open file");
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    QJsonObject obj = loadDoc.object();
    QJsonArray jarray = obj["table"].toArray();
    return jsonarray_to_table(console, lua, jarray);
};

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
}
/// @endcond

/*! \fn text propose_unique_filename_by_datetime(text dir_path, text prefix, text suffix)
    \brief Returns a filename which does not exist in the directory.
    \param dir_path                  The directory where to look for a file
    \param prefix                The prefix of the filename
    \param suffix                The suffix of the filename

    \return                the first free filename of the directory.

    \details    \par example:
        The Filename will be:
            dir/prefix_datetime_index_suffix
        where index is incremented until a free filename is found. Index is starting at 1. The Datetime is the datetime of now().

    \code{.lua}
        local retval = propose_unique_filename_by_datetime("~/test/","mytestfile",".txt")
        print(retval) --prints ~/test/mytestfile-2017_06_01-13_25_30-001.txt
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
text propose_unique_filename_by_datetime(text dir_path, text prefix, text suffix);
#endif

/// @cond HIDDEN_SYMBOLS
std::string propose_unique_filename_by_datetime(const std::string &dir_path, const std::string &prefix, const std::string &suffix) {
    int index = 0;
    QString dir_ = QString::fromStdString(dir_path);
    QString prefix_ = QString::fromStdString(prefix);
    QString suffix_ = QString::fromStdString(suffix);
    dir_ = QDir::toNativeSeparators(dir_);
    if (!dir_.endsWith(QDir::separator())) {
        dir_ += QDir::separator();
    }
    QString result;
    QString currentDateTime = QDateTime::currentDateTime().toString("-yyyy_MM_dd-HH_mm_ss-");
    //currentDateTime = "-2017_06_01-14_50_49-";
    do {
        index++;
        result = dir_ + prefix_ + currentDateTime + QString("%1").arg(index, 3, 10, QChar('0')) + suffix_;
    } while (QFile::exists(result));
    return result.toStdString();
}
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

/*! \fn pc_speaker_beep();
    \brief Sounds the bell, using the default volume and sound.

    \details    \par example:
    \code{.lua}
        pc_speaker_beep()
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
pc_speaker_beep();
#endif

/// @cond HIDDEN_SYMBOLS
void pc_speaker_beep() {
    QApplication::beep();
}
/// @endcond

/*! \fn git_info(string path, bool allow_modified)
    \brief returns the git revision hash of a given repository
    \param path                A String containing the path to the git repository
    \param allow_modified      If false this function will throw an exception if the repository contains modifications which are not yet commited.

    \return                    a table with the git-hash, the date and the modified-flag of the last commit.
    \details    \par example:
    \code{.lua}
        git = git_info(".",false)
        print(git) -- git.hash = "1e46a0f", git.modified = true, git.date = "2017-05-19 14:26:36 +0200"
    \endcode
*/

#ifdef DOXYGEN_ONLY
//this block is just for ducumentation purpose
string git_info(string path, bool allow_modified);
#endif

/// @cond HIDDEN_SYMBOLS
sol::table git_info(sol::state &lua, std::string path, bool allow_modified) {
    sol::table result = lua.create_table_with();
    bool modified = false;
    QString program = QSettings{}.value(Globals::git_path, "").toString();
    if (!QFile::exists(program)) {
        throw std::runtime_error("Could not find git executable at \"" + program.toStdString() + "\"");
    }
    if (!QFile::exists(QString::fromStdString(path))) {
        throw std::runtime_error("Could not find directory at \"" + path + "\"");
    }

    QList<QStringList> argument_list;
    argument_list.append({"status", "-s"});
    argument_list.append({"log", "--format=%h"});
    argument_list.append({"log", "--format=%ci"});

    for (uint i = 0; i < argument_list.count(); i++) {
        const auto &arguments = argument_list[i];
        QProcess myProcess(nullptr);
        myProcess.setWorkingDirectory(QString::fromStdString(path));

        myProcess.start(program, arguments);
        myProcess.waitForStarted();
        myProcess.waitForFinished();
        auto out = QString(myProcess.readAll()).split("\n");
        if (myProcess.exitCode() != 0) {
            throw std::runtime_error(myProcess.readAllStandardError().toStdString());
        }
        //qDebug() << out;
        switch (i) {
            case 0: {
                for (auto s : out) {
                    if (s.startsWith(" M")) {
                        modified = true;
                        break;
                    }
                }
                result["modfied"] = modified;
                if ((!allow_modified) && modified) {
                    throw std::runtime_error("Git Status: Git-Directory is modified.");
                }
            } break;
            case 1: {
                result["hash"] = out[0].toStdString();
            } break;
            case 2: {
                result["date"] = out[0].toStdString();
            } break;
            default: {
            } break; //
        }
    }
    return result;
}
/// @endcond
///
/** \} */ // end of group convenience
