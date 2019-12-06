#ifndef PROGRESSBAR_LUA_H
#define PROGRESSBAR_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_progressbar(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // PROGRESSBAR_LUA_H
