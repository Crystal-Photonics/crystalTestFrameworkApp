#include "datalogger_lua.h"
#include "datalogger.h"
#include "scriptengine.h"
#include "scriptsetup_helper.h"

void bind_datalogger(sol::state &lua, QPlainTextEdit *console, const std::string &path) {
	lua.new_usertype<DataLogger>(
		"DataLogger", sol::meta_function::construct,
		sol::factories([console = console, path = path](const std::string &file_name, char seperating_character, sol::table field_names, bool over_write_file) {
			abort_check();
			return DataLogger{console, get_absolute_file_path(QString::fromStdString(path), file_name), seperating_character, field_names, over_write_file};
		}),
		"append_data", wrap(&DataLogger::append_data), //
		"save", wrap(&DataLogger::save)                //
	);
}
