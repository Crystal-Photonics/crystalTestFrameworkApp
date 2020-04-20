#ifndef USERINSTRUCTIONLABEL_LUA_H
#define USERINSTRUCTIONLABEL_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_userinstructionlabel(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // USERINSTRUCTIONLABEL_LUA_H
