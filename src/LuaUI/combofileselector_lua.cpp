#include "combofileselector_lua.h"
#include "combofileselector.h"
#include "scriptengine.h"
#include "scriptsetup_helper.h"

void bind_combofileselector(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent, const std::string &path) {
	ui_table.new_usertype<Lua_UI_Wrapper<ComboBoxFileSelector>>(
		"ComboBoxFileSelector", //
		sol::meta_function::construct, sol::factories([parent = parent, path = path, &script_engine](const std::string &directory, const sol::table &filters) {
			abort_check();
			QStringList sl;
			for (const auto &item : filters) {
				sl.append(QString::fromStdString(item.second.as<std::string>()));
			}
			return Lua_UI_Wrapper<ComboBoxFileSelector>{parent, &script_engine, get_absolute_file_path(QString::fromStdString(path), directory), sl};
		}), //
		"get_selected_file",
		thread_call_wrapper(&ComboBoxFileSelector::get_selected_file),           //
		"set_visible", thread_call_wrapper(&ComboBoxFileSelector::set_visible),  //
		"set_order_by", thread_call_wrapper(&ComboBoxFileSelector::set_order_by) //
	);
}
