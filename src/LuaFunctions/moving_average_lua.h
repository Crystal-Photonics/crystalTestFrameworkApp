#ifndef MOVING_AVERAGE_LUA_H
#define MOVING_AVERAGE_LUA_H

#include <sol_forward.hpp>
#include <string>

class ScriptEngine;
class UI_container;
class QPlainTextEdit;

void bind_moving_average(sol::state &lua, QPlainTextEdit *console);

#endif // MOVING_AVERAGE_LUA_H
