#ifndef LUA_FUNCTIONS_H
#define LUA_FUNCTIONS_H

#include "sol.hpp"
#include <QString>
#include <vector>

class QPlainTextEdit;

std::vector<unsigned int> measure_noise_level_distribute_tresholds(const unsigned int length, const double min_val, const double max_val);
double measure_noise_level_czt(sol::state &lua, sol::table rpc_device, const unsigned int dacs_quantity, const unsigned int max_possible_dac_value);
void print(QPlainTextEdit *console, const sol::variadic_args &args);
std::string show_question(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message, sol::table button_table);
void show_info(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message);
void show_warning(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message);
void sleep_ms(const unsigned int timeout_ms);
void pc_speaker_beep();
double current_date_time_ms(void);
double round_double(const double value, const unsigned int precision);
void table_save_to_file(QPlainTextEdit *console, const std::string file_name, sol::table input_table, bool over_write_file);
sol::table table_load_from_file(QPlainTextEdit *console, sol::state &lua, const std::string file_name);
double table_sum(sol::table input_values);
double table_mean(sol::table input_values);
sol::table table_set_constant(sol::state &lua, sol::table input_values, double constant);
sol::table table_create_constant(sol::state &lua, const unsigned int size, double constant);
sol::table table_add_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_add_constant(sol::state &lua, sol::table input_values, double constant);
sol::table table_sub_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_mul_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_mul_constant(sol::state &lua, sol::table input_values_a, double constant);
sol::table table_div_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_round(sol::state &lua, sol::table input_values, const unsigned int precision);
sol::table table_abs(sol::state &lua, sol::table input_values);
sol::table table_mid(sol::state &lua, sol::table input_values, const unsigned int start, const unsigned int length);
bool table_equal_constant(sol::table input_values_a, double input_const_val);
bool table_equal_table(sol::table input_values_a, sol::table input_values_b);
double table_max(sol::table input_values);
double table_min(sol::table input_values);
double table_max_abs(sol::table input_values);
double table_min_abs(sol::table input_values);
sol::table git_info(sol::state &lua, std::string path, bool allow_modified);

#endif // LUA_FUNCTIONS_H
