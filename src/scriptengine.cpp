#include "scriptengine.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "mainwindow.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <vector>

ScriptEngine::ScriptEngine(LuaUI lua_ui)
    : lua_ui(std::make_unique<LuaUI>(std::move(lua_ui))) {}

void ScriptEngine::load_script(const QString &path) {
    //NOTE: When using lambdas do not capture this or by reference, because it breaks when the ScriptEngine is moved
    this->path = path;
    try {
        lua.open_libraries(sol::lib::base); //load the standard lib if necessary
        lua.set_function("show_warning", [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
            MainWindow::mw->show_message_box(QString::fromStdString(title.value_or("nil")) + " from " + path, QString::fromStdString(message.value_or("nil")), QMessageBox::Warning);
        });

        lua.set_function("sleep_ms", [](const unsigned int timeout_ms) { QThread::msleep(timeout_ms); });

        lua.script_file(path.toStdString());

        //bind UI
        auto ui_table = lua.create_named_table("ui");

        //bind plot
        ui_table.new_usertype<LuaPlot>("plot",                                                                                         //
                                       sol::meta_function::construct, sol::no_constructor,                                             //
                                       sol::meta_function::construct, [lua_ui = this->lua_ui.get()] { return lua_ui->create_plot(); }, //
                                       "add_point", &LuaPlot::add_point,                                                               //
                                       "add_spectrum",
                                       [](LuaPlot &plot, const sol::table &table) {
                                           std::vector<double> data;
                                           data.reserve(table.size());
                                           for (std::size_t i = 0; i < table.size(); i++) {
                                               data.push_back(table[i]);
                                           }
                                           plot.add_spectrum(data);
                                       }, //
                                       "add_spectrum",
                                       [](LuaPlot &plot, const unsigned int spectrum_start_channel, const sol::table &table) {
                                           std::vector<double> data;
                                           data.reserve(table.size());
                                           for (std::size_t i = 0; i < table.size(); i++) {
                                               data.push_back(table[i]);
                                           }
                                           plot.add_spectrum(spectrum_start_channel, data);
                                       }, //
                                       "clear",
                                       &LuaPlot::clear,                    //
                                       "set_offset", &LuaPlot::set_offset, //
                                       "set_gain", &LuaPlot::set_gain);
        //bind button
        ui_table.new_usertype<LuaButton>("button", //
                                         sol::meta_function::construct,
                                         [lua_ui = this->lua_ui.get()](const std::string &title) { return lua_ui->create_button(title); }, //
                                         "has_been_pressed", &LuaButton::has_been_pressed);

    } catch (const sol::error &error) {
        set_error(error);
        throw;
    }
}

QStringList ScriptEngine::get_string_list(const QString &name) {
    QStringList retval;
    sol::table t = lua.get<sol::table>(name.toStdString());
    try {
        if (t.valid() == false) {
            return retval;
        }
        for (auto &s : t) {
            retval << s.second.as<std::string>().c_str();
        }
    } catch (const sol::error &error) {
        set_error(error);
        throw;
    }
    return retval;
}

void ScriptEngine::set_error(const sol::error &error) {
    const std::string &string = error.what();
    std::regex r(R"(\.lua:([0-9]*): )");
    std::smatch match;
    if (std::regex_search(string, match, r)) {
        Utility::convert(match[1].str(), error_line);
    }
}

void ScriptEngine::launch_editor() const {
    QStringList parameter;
    if (error_line != 0) {
        parameter << path + ":" + QString::number(error_line);
    } else {
        parameter << path;
    }
    QProcess::startDetached(QSettings{}.value(Globals::lua_editor_path_settings_key, R"(C:\Qt\Tools\QtCreator\bin\qtcreator.exe)").toString(), parameter);
}

LuaUI &ScriptEngine::get_ui() {
    return *lua_ui;
}

sol::table ScriptEngine::create_table() {
    return lua.create_table_with();
}

static sol::object create_lua_object_from_RPC_answer(const RPCRuntimeDecodedParam &param, sol::state &lua) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            auto array = param.as_array();
            if (array.size() == 1) {
                return create_lua_object_from_RPC_answer(array.front(), lua);
            }
            auto table = lua.create_table_with();
            for (std::size_t i = 0; i < array.size(); i++) {
                table[i] = create_lua_object_from_RPC_answer(array[i], lua);
            }
            return table;
        }
        case RPCRuntimeParameterDescription::Type::character:
            return sol::make_object(lua.lua_state(), "TODO: Parse return value of type character");
        case RPCRuntimeParameterDescription::Type::enumeration:
            return sol::make_object(lua.lua_state(), "TODO: Parse return value of type enumeration");
        case RPCRuntimeParameterDescription::Type::structure:
            return sol::make_object(lua.lua_state(), "TODO: Parse return value of type structure");
        case RPCRuntimeParameterDescription::Type::integer:
            return sol::make_object(lua.lua_state(), param.as_integer());
    }
    assert("Invalid type of RPCRuntimeParameterDescription");
    return sol::nil;
}

struct RPCDevice {
    sol::object call_rpc_function(const std::string &name, const sol::variadic_args &va) {
        if (QThread::currentThread()->isInterruptionRequested() || engine->state == ScriptEngine::State::aborting) {
            throw sol::error("Abort Requested");
        }
        Console::note() << QString("\"%1\" called").arg(name.c_str());
        auto function = protocol->encode_function(name);
        int param_count = 0;
        for (auto &arg : va) {
            auto &param = function.get_parameter(param_count++);
            if (param.is_integral_type()) {
                param = arg.get<int>();
            } else {
                param = arg.get<std::string>();
            }
        }
        if (function.are_all_values_set()) {
            auto result = protocol->call_and_wait(function);

            if (result) {
                try {
                    auto output_params = result->get_decoded_parameters();
                    if (output_params.empty()) {
                        return sol::nil;
                    } else if (output_params.size() == 1) {
                        return create_lua_object_from_RPC_answer(output_params.front(), *lua);
                    }
                    //else: multiple variables, need to make a table
                    return sol::make_object(lua->lua_state(), "TODO: Not Implemented: Parse multiple return values");
                } catch (const sol::error &e) {
                    Console::error() << e.what();
                    throw;
                }
            }
            throw sol::error("Call to \"" + name + "\" failed due to timeout");
        }
        //not all values set, error
        throw sol::error("Failed calling function, missing parameters");
    }
    sol::state *lua = nullptr;
    RPCProtocol *protocol = nullptr;
    CommunicationDevice *device = nullptr;
    ScriptEngine *engine = nullptr;
};

void ScriptEngine::run(std::list<DeviceProtocol> device_protocols, std::function<void(std::list<DeviceProtocol> &)> debug_callback) {
    auto lua_state_resetter = [this] {
        lua.~state();
        new (&lua) sol::state();
        load_script(path);
    };
    try {
        std::lock_guard<std::mutex> state_lock(*state_is_idle);
        {
            auto device_list = lua.create_table_with();
            for (auto &device_protocol : device_protocols) {
                if (auto rpcp = dynamic_cast<RPCProtocol *>(&device_protocol.protocol)) {
                    device_list.add(RPCDevice{&lua, rpcp, &device_protocol.device, this});
                    auto type_reg = lua.create_simple_usertype<RPCDevice>();
                    for (auto &function : rpcp->get_description().get_functions()) {
                        const auto &function_name = function.get_function_name();
                        type_reg.set(function_name,
                                     [function_name](RPCDevice &device, const sol::variadic_args &va) { return device.call_rpc_function(function_name, va); });
                    }
                    const auto &type_name = "RPCDevice_" + rpcp->get_description().get_hash();
                    lua.set_usertype(type_name, type_reg);
                } else {
                    //TODO: other protocols
                    throw std::runtime_error("invalid protocol: " + device_protocol.protocol.type.toStdString());
                }
            }
            state = State::running;
            lua["run"](device_list);
            state = State::done;
            lua_state_resetter();
        }
    } catch (const sol::error &e) {
        debug_callback(device_protocols);
        set_error(e);
        state = State::done;
        lua_state_resetter();
        throw;
    }
}
