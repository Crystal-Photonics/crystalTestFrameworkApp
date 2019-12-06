#include "checkbox_lua.h"
#include "checkbox.h"
#include "scriptsetup_helper.h"

void bind_checkbox(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	//bind CheckBox
	ui_table.new_usertype<Lua_UI_Wrapper<CheckBox>>("CheckBox", //
													sol::meta_function::construct, sol::factories([parent = parent, &script_engine](const std::string &text) {
														abort_check();
														return Lua_UI_Wrapper<CheckBox>{parent, &script_engine, text};
													}), //
													"set_checked",
													thread_call_wrapper(&CheckBox::set_checked),                //
													"get_checked", thread_call_wrapper(&CheckBox::get_checked), //
													"set_visible", thread_call_wrapper(&CheckBox::set_visible), //
													"set_text", thread_call_wrapper(&CheckBox::set_text),       //
													"set_enabled", thread_call_wrapper(&CheckBox::set_enabled), //
													"get_text", thread_call_wrapper(&CheckBox::get_text));
}
