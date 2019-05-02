#ifndef SCRIPTSETUP_H
#define SCRIPTSETUP_H

#include "console.h"
#include <QString>
#include <memory>
#include <string>

struct Data_engine;
namespace sol {
    class state;
}

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

class ScriptEngine;

void script_setup(sol::state &lua, const std::string &path, ScriptEngine &script_engine);

#endif // SCRIPTSETUP_H
