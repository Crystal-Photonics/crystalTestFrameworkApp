#ifndef CHECKBOX_LUA_H
#define CHECKBOX_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_checkbox(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // CHECKBOX_LUA_H
