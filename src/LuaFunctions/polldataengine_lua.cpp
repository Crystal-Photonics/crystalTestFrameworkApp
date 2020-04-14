#include "polldataengine_lua.h"
#include "dataengineinput_lua.h"
#include "polldataengine.h"
#include "scriptsetup_helper.h"

void bind_polldataengine(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
	ui_table.new_usertype<Lua_UI_Wrapper<PollDataEngine>>("PollDataEngine", //
														  sol::meta_function::construct,
														  sol::factories([parent = parent, &script_engine](Data_engine_handle &handle, const sol::table items) {
															  abort_check();
															  QStringList sl;
															  for (const auto &item : items) {
																  sl.append(QString::fromStdString(item.second.as<std::string>()));
															  }
															  return Lua_UI_Wrapper<PollDataEngine>{parent, &script_engine, handle.data_engine.get(), sl};
														  }), //

														  "set_visible",
														  thread_call_wrapper(&PollDataEngine::set_visible),                //
														  "refresh", thread_call_wrapper(&PollDataEngine::refresh),         //
														  "set_enabled", thread_call_wrapper(&PollDataEngine::set_enabled), //
														  "is_in_range", thread_call_wrapper(&PollDataEngine::is_in_range)  //
	);
}
