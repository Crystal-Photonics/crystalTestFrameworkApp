#include "button_lua.h"
#include "button.h"
#include "scriptsetup_helper.h"

void bind_button(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	//bind button
	ui_table.new_usertype<Lua_UI_Wrapper<Button>>("Button", //
												  sol::meta_function::construct, sol::factories([parent = parent, &script_engine](const std::string &title) {
													  abort_check();
													  return Lua_UI_Wrapper<Button>{parent, &script_engine, &script_engine, title};
												  }), //
												  "has_been_clicked",
												  thread_call_wrapper(&Button::has_been_clicked),                       //
												  "set_visible", thread_call_wrapper(&Button::set_visible),             //
												  "set_enabled", thread_call_wrapper(&Button::set_enabled),             //
												  "reset_click_state", thread_call_wrapper(&Button::reset_click_state), //
												  "await_click", non_gui_call_wrapper(&Button::await_click)             //
	);
}
