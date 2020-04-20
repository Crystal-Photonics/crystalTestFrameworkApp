#ifndef IMAGE_LUA_H
#define IMAGE_LUA_H

#include <sol_forward.hpp>
#include <string>

class ScriptEngine;
class UI_container;

void bind_image(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent, const std::string &path);

#endif // IMAGE_LUA_H
