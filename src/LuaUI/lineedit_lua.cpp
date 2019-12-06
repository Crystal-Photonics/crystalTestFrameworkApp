#include "lineedit_lua.h"
#include "lineedit.h"
#include "scriptsetup_helper.h"

void bind_lineedit(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<LineEdit>>("LineEdit", //
													sol::meta_function::construct, sol::factories([parent = parent, &script_engine] {
														return Lua_UI_Wrapper<LineEdit>(parent, &script_engine, &script_engine);
													}),                                                                           //
													"set_placeholder_text", thread_call_wrapper(&LineEdit::set_placeholder_text), //
													"get_text", thread_call_wrapper(&LineEdit::get_text),                         //
													"set_text", thread_call_wrapper(&LineEdit::set_text),                         //
													"set_name", thread_call_wrapper(&LineEdit::set_name),                         //
													"get_name", thread_call_wrapper(&LineEdit::get_name),                         //
													"get_number", thread_call_wrapper(&LineEdit::get_number),                     //
													"get_caption", thread_call_wrapper(&LineEdit::get_caption),                   //
													"set_caption", thread_call_wrapper(&LineEdit::set_caption),                   //
													"set_enabled", thread_call_wrapper(&LineEdit::set_enabled),                   //
													"set_visible", thread_call_wrapper(&LineEdit::set_visible),                   //
													"set_focus", thread_call_wrapper(&LineEdit::set_focus),                       //
													"await_return", non_gui_call_wrapper(&LineEdit::await_return),                //
													"get_date", thread_call_wrapper(&LineEdit::get_date),                         //
													"set_date", thread_call_wrapper(&LineEdit::set_date),                         //
													"set_date_mode", thread_call_wrapper(&LineEdit::set_date_mode),               //
													"set_text_mode", thread_call_wrapper(&LineEdit::set_text_mode),               //
													"set_input_check", thread_call_wrapper(&LineEdit::set_input_check),           //
													"load_from_cache", non_gui_call_wrapper(&LineEdit::load_from_cache)           //
	);
}
