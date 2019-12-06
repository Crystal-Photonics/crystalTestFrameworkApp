#ifndef POLLDATAENGINE_LUA_H
#define POLLDATAENGINE_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_polldataengine(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // POLLDATAENGINE_LUA_H
