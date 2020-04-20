#ifndef LINEEDIT_LUA_H
#define LINEEDIT_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_lineedit(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // LINEEDIT_LUA_H
