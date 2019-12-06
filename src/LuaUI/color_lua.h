#ifndef COLOR_LUA_H
#define COLOR_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_color(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // COLOR_LUA_H
