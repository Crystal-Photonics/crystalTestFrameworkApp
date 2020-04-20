#ifndef BUTTON_LUA_H
#define BUTTON_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_button(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // BUTTON_LUA_H
