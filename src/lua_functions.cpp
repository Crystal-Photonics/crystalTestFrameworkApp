#include "lua_functions.h"
#include "console.h"
#include "mainwindow.h"
#include "scriptengine.h"
#include "util.h"

#include <QTimer>
#include <cmath>

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
