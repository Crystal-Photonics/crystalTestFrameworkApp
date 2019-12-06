#include "image_lua.h"
#include "image.h"
#include "scriptsetup_helper.h"

void bind_image(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent, const std::string &path) {
	ui_table.new_usertype<Lua_UI_Wrapper<Image>>("Image", sol::meta_function::construct, sol::factories([parent = parent, path = path, &script_engine]() {
													 abort_check();
													 return Lua_UI_Wrapper<Image>{parent, &script_engine, QString::fromStdString(path)}; //
												 }),
												 "load_image_file", thread_call_wrapper(&Image::load_image_file),       //
												 "set_maximum_width", thread_call_wrapper(&Image::set_maximum_width),   //
												 "set_maximum_height", thread_call_wrapper(&Image::set_maximum_height), //
												 "set_visible", thread_call_wrapper(&Image::set_visible)                //
	);
}
