#ifndef SPINBOX_LUA_H
#define SPINBOX_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_spinbox(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // SPINBOX_LUA_H
