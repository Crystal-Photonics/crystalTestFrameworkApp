#include "label_lua.h"
#include "label.h"
#include "scriptsetup_helper.h"

void bind_label(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<Label>>("Label", //
												 sol::meta_function::construct, sol::factories([parent = parent, &script_engine](const std::string &text) {
													 abort_check();
													 return Lua_UI_Wrapper<Label>{parent, &script_engine, text};
												 }), //
												 "set_text",
												 thread_call_wrapper(&Label::set_text),                       //
												 "set_enabled", thread_call_wrapper(&Label::set_enabled),     //
												 "set_visible", thread_call_wrapper(&Label::set_visible),     //
												 "set_font_size", thread_call_wrapper(&Label::set_font_size), //
												 "get_text", thread_call_wrapper(&Label::get_text));
}
