#ifndef SCPIPROTOCOL_LUA_H
#define SCPIPROTOCOL_LUA_H

#include <sol_forward.hpp>

class ScriptEngine;

void bind_scpiprotocol(sol::state &lua, ScriptEngine &script_engine);

#endif // SCPIPROTOCOL_LUA_H
