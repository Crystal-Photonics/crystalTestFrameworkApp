#include "userwaitlabel_lua.h"
#include "scriptsetup_helper.h"
#include "userwaitlabel.h"

void bind_userwaitlabel(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<UserWaitLabel>>("UserWaitLabel", //
														 sol::meta_function::construct,
														 sol::factories([parent = parent, &script_engine](const std::string &instruction_text) {
															 abort_check();
															 return Lua_UI_Wrapper<UserWaitLabel>{parent, &script_engine, &script_engine, instruction_text};
														 }),                                                                                      //
														 "set_enabled", thread_call_wrapper(&UserWaitLabel::set_enabled),                         //
														 "sleep_ms", non_gui_call_wrapper(&UserWaitLabel::sleep_ms),                              //
														 "show_progress_timer_ms", thread_call_wrapper(&UserWaitLabel::show_progress_timer_ms),   //
														 "set_current_progress_ms", thread_call_wrapper(&UserWaitLabel::set_current_progress_ms), //
														 "set_text", thread_call_wrapper(&UserWaitLabel::set_text)                                //
	);
}
