#ifndef ISOTOPESOURCESELECTOR_LUA_H
#define ISOTOPESOURCESELECTOR_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_isotopesourceselector(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);
#endif // ISOTOPESOURCESELECTOR_LUA_H
