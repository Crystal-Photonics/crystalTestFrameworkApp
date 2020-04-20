#ifndef USERWAITLABEL_LUA_H
#define USERWAITLABEL_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_userwaitlabel(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // USERWAITLABEL_LUA_H
