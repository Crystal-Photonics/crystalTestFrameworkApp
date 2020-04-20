#include "progressbar_lua.h"
#include "progressbar.h"
#include "scriptsetup_helper.h"

void bind_progressbar(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<ProgressBar>>("ProgressBar", //
													   sol::meta_function::construct, sol::factories([parent = parent, &script_engine]() {
														   abort_check();
														   return Lua_UI_Wrapper<ProgressBar>{parent, &script_engine};
													   }), //
													   "set_max_value",
													   thread_call_wrapper(&ProgressBar::set_max_value),                      //
													   "set_min_value", thread_call_wrapper(&ProgressBar::set_min_value),     //
													   "set_value", thread_call_wrapper(&ProgressBar::set_value),             //
													   "increment_value", thread_call_wrapper(&ProgressBar::increment_value), //
													   "set_visible", thread_call_wrapper(&ProgressBar::set_visible),         //
													   "set_caption", thread_call_wrapper(&ProgressBar::set_caption),         //
													   "get_caption", thread_call_wrapper(&ProgressBar::get_caption)          //
	);
}
