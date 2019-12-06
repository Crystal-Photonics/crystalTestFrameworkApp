#include "userinstructionlabel_lua.h"
#include "scriptsetup_helper.h"
#include "userinstructionlabel.h"

void bind_userinstructionlabel(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<UserInstructionLabel>>(
		"UserInstructionLabel", //
		sol::meta_function::construct, sol::factories([parent = parent, &script_engine](const std::string &instruction_text) {
			abort_check();
			return Lua_UI_Wrapper<UserInstructionLabel>{parent, &script_engine, &script_engine, instruction_text};
		}), //
		"await_event",
		non_gui_call_wrapper(&UserInstructionLabel::await_event),                                //
		"await_yes_no", non_gui_call_wrapper(&UserInstructionLabel::await_yes_no),               //
		"set_visible", thread_call_wrapper(&UserInstructionLabel::set_visible),                  //
		"set_enabled", thread_call_wrapper(&UserInstructionLabel::set_enabled),                  //
		"set_instruction_text", thread_call_wrapper(&UserInstructionLabel::set_instruction_text) //
	);
}
