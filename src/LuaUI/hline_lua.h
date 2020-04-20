#ifndef HLINE_LUA_H
#define HLINE_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_hline(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // HLINE_LUA_H
