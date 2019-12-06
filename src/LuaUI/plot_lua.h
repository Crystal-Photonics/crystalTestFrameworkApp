#ifndef PLOT_LUA_H
#define PLOT_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;
class UI_container;

void bind_plot(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent);

#endif // PLOT_LUA_H
