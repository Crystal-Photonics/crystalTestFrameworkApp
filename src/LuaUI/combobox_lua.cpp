#include "combobox_lua.h"
#include "combobox.h"
#include "scriptsetup_helper.h"

void bind_combobox(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<ComboBox>>("ComboBox", //
													sol::meta_function::construct, sol::factories([parent = parent, &script_engine](const sol::table &items) {
														abort_check();
														return Lua_UI_Wrapper<ComboBox>{parent, &script_engine, items};
													}), //
													"set_items",
													thread_call_wrapper(&ComboBox::set_items),                   //
													"get_text", thread_call_wrapper(&ComboBox::get_text),        //
													"set_index", thread_call_wrapper(&ComboBox::set_index),      //
													"get_index", thread_call_wrapper(&ComboBox::get_index),      //
													"set_visible", thread_call_wrapper(&ComboBox::set_visible),  //
													"set_name", thread_call_wrapper(&ComboBox::set_name),        //
													"set_caption", thread_call_wrapper(&ComboBox::set_caption),  //
													"get_caption", thread_call_wrapper(&ComboBox::get_caption),  //
													"set_editable", thread_call_wrapper(&ComboBox::set_editable) //
	);
}
