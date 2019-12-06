#ifndef LUA_FUNCTIONS_LUA_H
#define LUA_FUNCTIONS_LUA_H

#include <sol_forward.hpp>
#include <string>

class ScriptEngine;
class QPlainTextEdit;

void bind_lua_functions(sol::state &lua, sol::table &ui_table, const std::string &path, ScriptEngine &script_engine, QPlainTextEdit *console);

#endif // LUA_FUNCTIONS_LUA_H
