#ifndef LABEL_LUA_H
#define LABEL_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_label(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // LABEL_LUA_H
