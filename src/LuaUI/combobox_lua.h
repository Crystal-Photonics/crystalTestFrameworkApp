#ifndef COMBOBOX_LUA_H
#define COMBOBOX_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_combobox(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // COMBOBOX_LUA_H
