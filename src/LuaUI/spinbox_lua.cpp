#include "spinbox_lua.h"
#include "scriptsetup_helper.h"
#include "spinbox.h"

void bind_spinbox(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<SpinBox>>("SpinBox", //
												   sol::meta_function::construct, sol::factories([parent = parent, &script_engine]() {
													   abort_check();
													   return Lua_UI_Wrapper<SpinBox>{parent, &script_engine}; //
												   }),                                                         //
												   "get_value",
												   thread_call_wrapper(&SpinBox::get_value),                      //
												   "set_max_value", thread_call_wrapper(&SpinBox::set_max_value), //
												   "set_min_value", thread_call_wrapper(&SpinBox::set_min_value), //
												   "set_value", thread_call_wrapper(&SpinBox::set_value),         //
												   "set_visible", thread_call_wrapper(&SpinBox::set_visible),     //
												   "set_caption", thread_call_wrapper(&SpinBox::set_caption),     //
												   "get_caption", thread_call_wrapper(&SpinBox::get_caption)      //
	);
}
