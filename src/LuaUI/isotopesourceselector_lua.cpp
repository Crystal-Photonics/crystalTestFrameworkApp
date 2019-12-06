#include "isotopesourceselector_lua.h"
#include "isotopesourceselector.h"
#include "scriptsetup_helper.h"

void bind_isotopesourceselector(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<IsotopeSourceSelector>>("IsotopeSourceSelector", //
																 sol::meta_function::construct, sol::factories([parent = parent, &script_engine]() {
																	 abort_check();
																	 return Lua_UI_Wrapper<IsotopeSourceSelector>{parent, &script_engine};
																 }), //
																 "set_visible",
																 thread_call_wrapper(&IsotopeSourceSelector::set_visible),                            //
																 "set_enabled", thread_call_wrapper(&IsotopeSourceSelector::set_enabled),             //
																 "filter_by_isotope", thread_call_wrapper(&IsotopeSourceSelector::filter_by_isotope), //
																 "get_selected_activity_Bq",
																 thread_call_wrapper(&IsotopeSourceSelector::get_selected_activity_Bq),               //
																 "get_selected_name", thread_call_wrapper(&IsotopeSourceSelector::get_selected_name), //
																 "get_selected_serial_number",
																 thread_call_wrapper(&IsotopeSourceSelector::get_selected_serial_number) //
	);
}
