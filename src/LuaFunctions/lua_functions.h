#ifndef LUA_FUNCTIONS_H
#define LUA_FUNCTIONS_H

#include "scriptengine.h"
#include "sol.hpp"

#include <QString>
#include <vector>

class QPlainTextEdit;

QStringList get_lua_lib_search_paths_for_lua(const QString &script_path, const QString &lua_search_pattern);

QString create_path(QString filename);
QString get_clean_file_path(QString filename);
QString append_separator_to_path(QString path);
QStringList get_search_path_entries(QString search_path);
QString search_in_search_path(const QString &script_path, const QString &file_to_be_searched);
QString get_search_paths(const QString &script_path);
std::vector<unsigned int> measure_noise_level_distribute_tresholds(const unsigned int length, const double min_val, const double max_val);
double measure_noise_level_czt(sol::state &lua, sol::table rpc_device, const unsigned int dacs_quantity, const unsigned int max_possible_dac_value);
void print(QPlainTextEdit *console, const sol::variadic_args &args);
std::string show_file_save_dialog(const std::string &title, const std::string &path, sol::table filters);
std::string show_file_open_dialog(const std::string &title, const std::string &path, sol::table filters);
std::string show_question(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message, sol::table button_table);
void show_info(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message);
void show_warning(const QString &path, const sol::optional<std::string> &title, const sol::optional<std::string> &message);
void sleep_ms(ScriptEngine *scriptengine, const unsigned int duration_ms, const unsigned int starttime_ms);
void pc_speaker_beep();
QString run_external_tool(const QString &script_path, const QString &execute_directory, const QString &executable, const sol::table &arguments, uint timeout);
double current_date_time_ms(void);
double round_double(const double value, const unsigned int precision);
void table_save_to_file(QPlainTextEdit *console, const std::string file_name, sol::table input_table, bool over_write_file);
sol::table table_load_from_file(QPlainTextEdit *console, sol::state &lua, const std::string file_name);
uint16_t table_crc16(QPlainTextEdit *console, sol::table input_values);
uint table_find_string(sol::table input_table, std::string search_text);
bool table_contains_string(sol::table input_table, std::string search_text);

double table_sum(sol::table input_values);
double table_mean(sol::table input_values);
double table_variance(sol::table input_values);
double table_standard_deviation(sol::table input_values);
sol::table table_set_constant(sol::state &lua, sol::table input_values, double constant);
sol::table table_create_constant(sol::state &lua, const unsigned int size, double constant);
sol::table table_add_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_add_table_at(sol::state &lua, sol::table input_values_a, sol::table input_values_b, unsigned int at);
sol::table table_add_constant(sol::state &lua, sol::table input_values, double constant);
sol::table table_sub_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_mul_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_mul_constant(sol::state &lua, sol::table input_values_a, double constant);
sol::table table_div_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
sol::table table_round(sol::state &lua, sol::table input_values, const unsigned int precision);
sol::table table_abs(sol::state &lua, sol::table input_values);
sol::table table_mid(sol::state &lua, sol::table input_values, const unsigned int start, const unsigned int length);
sol::table table_range(sol::state &lua, double start, double stop, double step);
sol::table table_concat(sol::state &lua, sol::table table1, sol::table table2);
bool table_equal_constant(sol::table input_values_a, double input_const_val);
bool table_equal_table(sol::state &lua, sol::table input_values_a, sol::table input_values_b);
double table_max(sol::table input_values);
double table_min(sol::table input_values);
double table_max_abs(sol::table input_values);
double table_min_abs(sol::table input_values);
sol::object table_max_by_field(sol::state &lua, sol::table input_values, const std::string field_name);
sol::object table_min_by_field(sol::state &lua, sol::table input_values, const std::string field_name);
QDateTime decode_date_time_from_file_name(const std::string &file_name, const std::string &prefix);
std::string propose_unique_filename_by_datetime(const std::string &dir_path, const std::string &prefix, const std::string &suffix);
sol::table git_info(sol::state &lua, std::string path, bool allow_modified);
QMap<QString, QVariant> git_info(QString path, bool allow_modified, bool allow_exceptions);
std::string get_framework_git_hash();
double get_framework_git_date_unix();
std::string get_framework_git_date_text();
std::string get_os_username();
void mk_link(std::string link_pointing_to, std::string link_name);
std::string file_link_points_to(std::string link_name);
bool path_exists(std::string path);
bool is_file_path_equal(std::string file_path_a, std::string file_path_b);

#endif // LUA_FUNCTIONS_H
