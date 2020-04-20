#ifndef DATALOGGER_LUA_H
#define DATALOGGER_LUA_H

#include <sol_forward.hpp>
#include <string>

class ScriptEngine;
class UI_container;
class QPlainTextEdit;

void bind_datalogger(sol::state &lua, QPlainTextEdit *console, const std::string &path);

#endif // DATALOGGER_LUA_H
