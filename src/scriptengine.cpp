#include "scriptengine.h"
#include "LuaUI/button.h"
#include "LuaUI/lineedit.h"
#include "LuaUI/plot.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "mainwindow.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <vector>

template <class T>
struct Lua_UI_Wrapper {
    template <class... Args>
    Lua_UI_Wrapper(QSplitter *parent, Args &&... args) {
        Utility::thread_call(MainWindow::mw, [ id = id, parent, args... ] { MainWindow::mw->add_lua_UI_class<T>(id, parent, args...); });
    }
    Lua_UI_Wrapper(Lua_UI_Wrapper &&other)
        : id(other.id) {
        other.id = -1;
    }
    Lua_UI_Wrapper &operator=(Lua_UI_Wrapper &&other) {
        std::swap(id, other.id);
    }
    ~Lua_UI_Wrapper() {
        if (id != -1) {
            Utility::thread_call(MainWindow::mw, [id = this->id] { MainWindow::mw->remove_lua_UI_class<T>(id); });
        }
    }

    int id = id_counter++;

    private:
    static int id_counter;
};

template <class T>
int Lua_UI_Wrapper<T>::id_counter;

namespace detail {
    //this might be replacable by std::invoke once C++17 is available
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs, std::size_t... I>
    ReturnType call_helper(ReturnType (UI_class::*func)(Args...), UI_class &ui, Params<ParamArgs...> const &params, std::index_sequence<I...>) {
        return (ui.*func)(std::get<I>(params)...);
    }
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs, std::size_t... I>
    ReturnType call_helper(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, Params<ParamArgs...> const &params, std::index_sequence<I...>) {
        return (ui.*func)(std::get<I>(params)...);
    }

    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs>
    ReturnType call(ReturnType (UI_class::*func)(Args...), UI_class &ui, Params<ParamArgs...> const &params) {
        return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
    }
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs>
    ReturnType call(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, Params<ParamArgs...> const &params) {
        return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
    }
}

template <class ReturnType, class UI_class, class... Args>
auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...)) {
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
        //TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        return Utility::promised_thread_call(MainWindow::mw, [ function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        });
    };
}

template <class ReturnType, class UI_class, class... Args>
auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...) const) {
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
        //TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        return Utility::promised_thread_call(MainWindow::mw, [ function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        });
    };
}

ScriptEngine::ScriptEngine(QSplitter *parent, QPlainTextEdit *console)
    : parent(parent)
    , console(console) {}

ScriptEngine::~ScriptEngine() {}

void ScriptEngine::load_script(const QString &path) {
    //NOTE: When using lambdas do not capture `this` or by reference, because it breaks when the ScriptEngine is moved
    this->path = path;
    try {
        //load the standard libs if necessary
        lua.open_libraries();

        //add generic function
        lua["show_warning"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
            MainWindow::mw->show_message_box(QString::fromStdString(title.value_or("nil")) + " from " + path, QString::fromStdString(message.value_or("nil")),
                                             QMessageBox::Warning);
        };
        lua["print"] = [console = console](const sol::variadic_args &args) {
            std::string text;
            for (auto &object : args) {
                if (sol::optional<int> i = object) {
                    text = std::to_string(i.value());
                } else if (sol::optional<double> d = object) {
                    text = std::to_string(d.value());
                } else if (sol::optional<std::string> s = object) {
                    text = s.value();
                } else {
                    assert(!"TODO: add types to print");
                }
            }
            Utility::thread_call(MainWindow::mw, [ console = console, text = std::move(text) ] { Console::script(console) << text; });
        };
        lua["sleep_ms"] = [](const unsigned int timeout_ms) {
            QEventLoop event_loop;
            static const auto secret_exit_code = -0xF42F;
            QTimer::singleShot(timeout_ms, [&event_loop] { event_loop.exit(secret_exit_code); });
            auto exit_value = event_loop.exec();
            if (exit_value != secret_exit_code) {
                throw sol::error("Interrupted");
            }
        };

        lua["round"] = [](const double value, const unsigned int precision=0 ) {
            double faktor =  pow(10,precision);
            double retval = value;
            retval *= faktor;
            retval = round(retval);
            return retval/faktor;
        };

        lua.script_file(path.toStdString());

        //bind UI
        auto ui_table = lua.create_named_table("Ui");

        //bind plot
        ui_table.new_usertype<Lua_UI_Wrapper<Plot>>("Plot",                                                                                          //
                                                    sol::meta_function::construct, [parent = this->parent] { return Lua_UI_Wrapper<Plot>{parent}; }, //
                                                    "add_point", thread_call_wrapper<void, Plot, double, double>(&Plot::add),                        //
                                                    "add_spectrum",
                                                    [](Lua_UI_Wrapper<Plot> &plot, sol::table table) {
                                                        std::vector<double> data;
                                                        data.reserve(table.size());
                                                        for (auto &i : table) {
                                                            data.push_back(i.second.as<double>());
                                                        }
                                                        Utility::thread_call(MainWindow::mw, [ id = plot.id, data = std::move(data) ] {
                                                            auto &plot = MainWindow::mw->get_lua_UI_class<Plot>(id);
                                                            plot.add(data);
                                                        });
                                                    }, //
                                                    "add_spectrum_at",
                                                    [](Lua_UI_Wrapper<Plot> &plot, const unsigned int spectrum_start_channel, const sol::table &table) {
                                                        std::vector<double> data;
                                                        data.reserve(table.size());
                                                        for (auto &i : table) {
                                                            data.push_back(i.second.as<double>());
                                                        }
                                                        Utility::thread_call(MainWindow::mw, [ id = plot.id, data = std::move(data), spectrum_start_channel ] {
                                                            auto &plot = MainWindow::mw->get_lua_UI_class<Plot>(id);
                                                            plot.add(spectrum_start_channel, data);
                                                        });
                                                    }, //
                                                    "clear",
                                                    thread_call_wrapper(&Plot::clear),                    //
                                                    "set_offset", thread_call_wrapper(&Plot::set_offset), //
                                                    "set_enable_median", thread_call_wrapper(&Plot::set_enable_median),           //
                                                    "set_median_kernel_size", thread_call_wrapper(&Plot::set_median_kernel_size), //
                                                    "integrate_ci", thread_call_wrapper(&Plot::integrate_ci),                     //
                                                    "set_gain", thread_call_wrapper(&Plot::set_gain));
        //bind button
        ui_table.new_usertype<Lua_UI_Wrapper<Button>>("Button", //
                                                      sol::meta_function::construct,
                                                      [parent = this->parent](const std::string &title) {
                                                          return Lua_UI_Wrapper<Button>{parent, title};
                                                      }, //
                                                      "has_been_pressed",
                                                      thread_call_wrapper(&Button::has_been_pressed) //
                                                      );
        //bind edit field
        ui_table.new_usertype<Lua_UI_Wrapper<LineEdit>>("LineEdit",                                                                                          //
                                                        sol::meta_function::construct, [parent = this->parent] { return Lua_UI_Wrapper<LineEdit>(parent); }, //
                                                        "set_placeholder_text", thread_call_wrapper(&LineEdit::set_placeholder_text),                        //
                                                        "get_text", thread_call_wrapper(&LineEdit::get_text),                                                //
                                                        "await_return",
                                                        [](const Lua_UI_Wrapper<LineEdit> &lew) {
                                                            auto le = MainWindow::mw->get_lua_UI_class<LineEdit>(lew.id);
                                                            le.set_single_shot_return_pressed_callback([thread = QThread::currentThread()] { thread->exit(); });
                                                            QEventLoop loop;
                                                            loop.exec();
                                                            auto text = Utility::promised_thread_call(MainWindow::mw, [&le] { return le.get_text(); });
                                                            return text;
                                                        } //
                                                        );
    } catch (const sol::error &error) {
        set_error(error);
        throw;
    }
}

void ScriptEngine::set_error(const sol::error &error) {
    const std::string &string = error.what();
    std::regex r(R"(\.lua:([0-9]*): )");
    std::smatch match;
    if (std::regex_search(string, match, r)) {
        Utility::convert(match[1].str(), error_line);
    }
}

void ScriptEngine::launch_editor(QString path, int error_line) {
    auto editor = QSettings{}.value(Globals::lua_editor_path_settings_key, R"(C:\Qt\Tools\QtCreator\bin\qtcreator.exe)").toString();
    auto parameters = QSettings{}.value(Globals::lua_editor_parameters_settings_key, R"(%1)").toString().split(" ");
    for (auto &parameter : parameters) {
        parameter = parameter.replace("%1", path).replace("%2", QString::number(error_line));
    }
    QProcess::startDetached(editor, parameters);
}

void ScriptEngine::launch_editor() const {
    launch_editor(path, error_line);
}

sol::table ScriptEngine::create_table() {
    return lua.create_table_with();
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

static sol::object create_lua_object_from_RPC_answer(const RPCRuntimeDecodedParam &param, sol::state &lua) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            auto array = param.as_array();
            if (array.size() == 1) {
                return create_lua_object_from_RPC_answer(array.front(), lua);
            }
            auto table = lua.create_table_with();
            for (std::size_t i = 0; i < array.size(); i++) {
                table.add(create_lua_object_from_RPC_answer(array[i], lua));
            }
            return table;
        }
        case RPCRuntimeParameterDescription::Type::character:
            throw sol::error("TODO: Parse return value of type character");
        case RPCRuntimeParameterDescription::Type::enumeration:
            return sol::make_object(lua.lua_state(), param.as_enum().value);
        case RPCRuntimeParameterDescription::Type::structure:
            throw sol::error("TODO: Parse return value of type structure");
        case RPCRuntimeParameterDescription::Type::integer:
            return sol::make_object(lua.lua_state(), param.as_integer());
    }
    assert(!"Invalid type of RPCRuntimeParameterDescription");
    return sol::nil;
}

struct RPCDevice {
    sol::object call_rpc_function(const std::string &name, const sol::variadic_args &va) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            throw sol::error("Abort Requested");
        }
        lua->collect_garbage();
        //qDebug() << QString("lua memory used ") + QString::number(lua->memory_used()/1024) + QString("kb\n");

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
            if (name == "get_spectrum_recording") {
                auto table = lua->create_table_with();
                for (int64_t i = 0; i < 256; i++) {
                    table.add(sol::make_object(lua->lua_state(), i));
                }
                return table;
            }
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

void add_enum_type(const RPCRuntimeParameterDescription &param, sol::state &lua) {
    if (param.get_type() == RPCRuntimeParameterDescription::Type::enumeration) {
        const auto &enum_description = param.as_enumeration();
        auto table = lua.create_named_table(enum_description.enum_name);
        for (auto &value : enum_description.values) {
            table[value.name] = value.to_int();
            table["to_string"] = [enum_description](int enum_value_param) -> std::string {
                for (const auto &enum_value : enum_description.values) {
                    if (enum_value.to_int() == enum_value_param) {
                        return enum_value.name;
                    }
                }
                return "";
            };
        }
    }
}

void add_enum_types(const RPCRuntimeFunction &function, sol::state &lua) {
    for (auto &param : function.get_request_parameters()) {
        add_enum_type(param, lua);
    }
    for (auto &param : function.get_reply_parameters()) {
        add_enum_type(param, lua);
    }
}

void ScriptEngine::run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices) {
    auto reset_lua_state = [this] {
        sol::state default_state;
        std::swap(lua, default_state);
        load_script(path);
    };
    try {
        {
            auto device_list = lua.create_table_with();
            for (auto &device_protocol : devices) {
                if (auto rpcp = dynamic_cast<RPCProtocol *>(device_protocol.second)) {
                    device_list.add(RPCDevice{&lua, rpcp, device_protocol.first, this});
                    auto type_reg = lua.create_simple_usertype<RPCDevice>();
                    for (auto &function : rpcp->get_description().get_functions()) {
                        const auto &function_name = function.get_function_name();
                        type_reg.set(function_name,
                                     [function_name](RPCDevice &device, const sol::variadic_args &va) { return device.call_rpc_function(function_name, va); });
                        add_enum_types(function, lua);
                    }
                    const auto &type_name = "RPCDevice_" + rpcp->get_description().get_hash();
                    lua.set_usertype(type_name, type_reg);
                } else {
                    //TODO: other protocols
                    throw std::runtime_error("invalid protocol: " + device_protocol.second->type.toStdString());
                }
            }
            lua["run"](device_list);
        }
        reset_lua_state();
    } catch (const sol::error &e) {
        set_error(e);
        reset_lua_state();
        throw;
    }
}
