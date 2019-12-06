#include "hline_lua.h"
#include "hline.h"
#include "scriptsetup_helper.h"

void bind_hline(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<HLine>>("HLine", //
												 sol::meta_function::construct, sol::factories([parent = parent, &script_engine]() {
													 abort_check();
													 return Lua_UI_Wrapper<HLine>{parent, &script_engine};
												 }), //
												 "set_visible", thread_call_wrapper(&HLine::set_visible));
}
