#ifndef DATAENGINEINPUT_LUA_H
#define DATAENGINEINPUT_LUA_H

#include <QString>
#include <memory>
#include <sol_forward.hpp>
#include <string>

class ScriptEngine;
class UI_container;
class Console;
class Data_engine;
///\cond HIDDEN_SYMBOLS
void bind_dataengineinput(sol::state &lua, sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent, const std::string &path);

struct Data_engine_handle {
    std::unique_ptr<Data_engine> data_engine;
    Data_engine_handle() = delete;
    Data_engine_handle(std::unique_ptr<Data_engine> de, Console &console)
        : data_engine{std::move(de)}
        , console{&console} {}
    Data_engine_handle(Data_engine_handle &&) = default;

    QString data_engine_auto_dump_path;
    QString additional_pdf_path;
    QString data_engine_pdf_template_path;
    Console *console;
    ~Data_engine_handle();
};
/// \endcond

#endif // DATAENGINEINPUT_LUA_H
