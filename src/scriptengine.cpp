#include "scriptengine.h"
#include "LuaUI/button.h"
#include "LuaUI/checkbox.h"
#include "LuaUI/color.h"
#include "LuaUI/combobox.h"
#include "LuaUI/combofileselector.h"
#include "LuaUI/dataengineinput.h"
#include "LuaUI/hline.h"
#include "LuaUI/image.h"
#include "LuaUI/isotopesourceselector.h"
#include "LuaUI/label.h"
#include "LuaUI/lineedit.h"
#include "LuaUI/plot.h"
#include "LuaUI/progressbar.h"
#include "LuaUI/spinbox.h"
#include "Protocols/manualprotocol.h"
#include "Protocols/rpcprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "Protocols/sg04countprotocol.h"
#include "Windows/mainwindow.h"
#include "chargecounter.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h"
#include "datalogger.h"
#include "lua_functions.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "ui_container.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSettings>
#include <QShortcut>
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
    Lua_UI_Wrapper(UI_container *parent, Args &&... args) {
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

    //list of types useful for returning a variadic template parameter pack or inspecting some of the parameters
    template <class... Args>
    struct Type_list;
    template <class Head_, class... Tail>
    struct Type_list<Head_, Tail...> {
        using Head = Head_;
        using Next = Type_list<Tail...>;
        static constexpr auto size = sizeof...(Tail) + 1;
    };
    template <>
    struct Type_list<> {
        static constexpr auto size = 0;
    };

    //allows conversion from sol::object into int, std::string, ...
    struct Converter {
        Converter(sol::object object)
            : object{object} {}
        sol::object object;
        template <class T>
        operator T() {
            return object.as<T>();
        }
    };

    //call a function with a list of sol::objects as the arguments
    template <class Function, std::size_t... indexes>
    auto call(std::vector<sol::object> &objects, Function &&function, std::index_sequence<indexes...>) {
        return function(Converter(objects[indexes])...);
    }

    //check if a list of sol::objects is convertible to a given variadic template parameter pack
    template <int index>
    bool is_convertible(std::vector<sol::object> &) {
        return true;
    }
    template <int index, class Head, class... Tail>
    bool is_convertible(std::vector<sol::object> &objects) {
        if (objects[index].is<Head>()) {
            return is_convertible<index + 1, Tail...>(objects);
        }
        return false;
    }
    template <class... Args>
    bool is_convertible(Type_list<Args...>, std::vector<sol::object> &objects) {
        return is_convertible<0, Args...>(objects);
    }

    //information for callable objects, such as number of parameters, parameter types and return type.
    //Fails to compile if the list is ambiguous due to overloading.
    template <class Return_type_, bool is_class_member_, class Parent_class_, class... Parameters_>
    struct Callable_info {
        using Return_type = Return_type_;
        using Parameters = Type_list<Parameters_...>;
        constexpr static bool is_class_member = is_class_member_;
        using Parent_class = Parent_class_;
    };
    template <class ReturnType, class... Args>
    Callable_info<ReturnType, false, void, Args...> callable_info(ReturnType (*)(Args...));
    template <class ReturnType, class Class, class... Args>
    Callable_info<ReturnType, true, Class, Args...> callable_info(ReturnType (Class::*)(Args...));
    template <class ReturnType, class Class, class... Args>
    Callable_info<ReturnType, true, Class, Args...> callable_info(ReturnType (Class::*)(Args...) const);
    template <class Function>
    auto callable_info(Function) -> decltype(callable_info(&Function::operator()));

    template <class Function>
    using Pointer_to_callable_t = std::add_pointer_t<std::remove_reference_t<Function>>;

    //get the parameter list of a callable object. Fails to compile if the list is ambiguous due to overloading.
    template <class Function>
    auto get_parameter_list(Function &&function) -> typename decltype(callable_info(function))::Parameters;
    template <class Function>
    using parameter_list_t = decltype(get_parameter_list(*Pointer_to_callable_t<Function>{}));

    //get the return type of a callable object. Fails to compile if the list is ambiguous due to overloading.
    template <class Function>
    auto get_return_type(Function &&function) -> typename decltype(callable_info(function))::Return_type;
    template <class Function>
    using return_type_t = decltype(get_return_type(*Pointer_to_callable_t<Function>{}));

    //get number of parameters of a callable. Fails to compile when ambiguous for overloaded callables.
    template <class Function>
    constexpr auto number_of_parameters{decltype(get_parameter_list(*Pointer_to_callable_t<Function>{}))::size};

    //call a function if it is callable with the given sol::objects
    template <class ReturnType>
    ReturnType try_call(std::false_type /*return void*/, std::vector<sol::object> &) {
        throw std::runtime_error("Invalid arguments for overloaded function call, none of the functions could handle the given arguments");
    }
    template <class ReturnType>
    void try_call(std::true_type /*return void*/, std::vector<sol::object> &) {
        throw std::runtime_error("Invalid arguments for overloaded function call, none of the functions could handle the given arguments");
    }
    template <class ReturnType, class FunctionHead, class... FunctionsTail>
    ReturnType try_call(std::false_type /*return void*/, std::vector<sol::object> &objects, FunctionHead &&function, FunctionsTail &&... functions) {
        constexpr auto arity = detail::number_of_parameters<FunctionHead>;
        //skip function if it has the wrong number of parameters
        if (arity != objects.size()) {
            return try_call<ReturnType>(std::false_type{}, objects, std::forward<FunctionsTail>(functions)...);
        }
        //skip function if the argument types don't match
        if (is_convertible(parameter_list_t<FunctionHead>{}, objects)) {
            return call(objects, std::forward<FunctionHead>(function), std::make_index_sequence<arity>());
        }
        //give up and try the next overload
        return try_call<ReturnType>(std::false_type{}, objects, functions...);
    }
    template <class ReturnType, class FunctionHead, class... FunctionsTail>
    void try_call(std::true_type /*return void*/, std::vector<sol::object> &objects, FunctionHead &&function, FunctionsTail &&... functions) {
        constexpr auto arity = detail::number_of_parameters<FunctionHead>;
        //skip function if it has the wrong number of parameters
        if (arity != objects.size()) {
            return try_call<ReturnType>(std::true_type{}, objects, std::forward<FunctionsTail>(functions)...);
        }
        //skip function if the argument types don't match
        if (is_convertible(parameter_list_t<FunctionHead>{}, objects)) {
            call(objects, std::forward<FunctionHead>(function), std::make_index_sequence<arity>());
            return;
        }
        //give up and try the next overload
        return try_call<ReturnType>(std::true_type{}, objects, functions...);
    }

    template <class ReturnType, class... Functions>
    auto overloaded_function_helper(std::false_type /*should_returntype_be_deduced*/, Functions &&... functions) {
        return [functions...](sol::variadic_args args) {
            std::vector<sol::object> objects;
            for (auto object : args) {
                objects.push_back(std::move(object));
            }
            return try_call<ReturnType>(std::is_same<ReturnType, void>(), objects, functions...);
        };
    }

    template <class ReturnType, class Functions_head, class... Functions_tail>
    auto overloaded_function_helper(std::true_type /*should_returntype_be_deduced*/, Functions_head &&functions_head, Functions_tail &&... functions_tail) {
        return overloaded_function_helper<return_type_t<Functions_head>>(std::false_type{}, std::forward<Functions_head>(functions_head),
                                                                         std::forward<Functions_tail>(functions_tail)...);
    }
}

//wrapper that wraps a UI function such as Button::has_been_clicked so that it is called from the main window context. Waits for processing.
template <class ReturnType, class UI_class, class... Args>
auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...)) {
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw sol::error("Abort Requested");
    }
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
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw sol::error("Abort Requested");
    }
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

//wrapper that wraps a UI function such as Button::has_been_clicked so that it is called from the main window context. Doesn't wait for processing.
template <class ReturnType, class UI_class, class... Args>
auto thread_call_wrapper_non_waiting(ReturnType (UI_class::*function)(Args...)) {
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw sol::error("Abort Requested");
    }
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
        Utility::thread_call(MainWindow::mw, [ function, id = lui.id, args = std::make_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        });
    };
}

//create an overloaded function from a list of functions. When called the overloaded function will pick one of the given functions based on arguments.
template <class ReturnType = std::false_type, class... Functions>
auto overloaded_function(Functions &&... functions) {
    return detail::overloaded_function_helper<ReturnType>(typename std::is_same<ReturnType, std::false_type>::type{}, std::forward<Functions>(functions)...);
}

static sol::object create_lua_object_from_RPC_answer(const RPCRuntimeDecodedParam &param, sol::state &lua) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            auto array = param.as_array();
            if (array.size() == 1) {
                return create_lua_object_from_RPC_answer(array.front(), lua);
            }
            auto table = lua.create_table_with();
            for (auto &element : array) {
                table.add(create_lua_object_from_RPC_answer(element, lua));
            }
            return table;
        }
        case RPCRuntimeParameterDescription::Type::character:
            throw sol::error("TODO: Parse return value of type character");
        case RPCRuntimeParameterDescription::Type::enumeration:
            return sol::make_object(lua.lua_state(), param.as_enum().value);
        case RPCRuntimeParameterDescription::Type::structure: {
            auto table = lua.create_table_with();
            for (auto &element : param.as_struct()) {
                table[element.name] = create_lua_object_from_RPC_answer(element.type, lua);
            }
            return table;
        }
        case RPCRuntimeParameterDescription::Type::integer:
            return sol::make_object(lua.lua_state(), param.as_integer());
    }
    assert(!"Invalid type of RPCRuntimeParameterDescription");
    return sol::nil;
}

static void set_runtime_parameter(RPCRuntimeEncodedParam &param, sol::object object) {
    if (param.get_description()->get_type() == RPCRuntimeParameterDescription::Type::array && param.get_description()->as_array().number_of_elements == 1) {
        return set_runtime_parameter(param[0], object);
    }
    switch (object.get_type()) {
        case sol::type::boolean:
            param.set_value(object.as<bool>() ? 1 : 0);
            break;
        case sol::type::function:
            throw sol::error("Cannot pass an object of type function to RPC");
        case sol::type::number:
            param.set_value(object.as<int64_t>());
            break;
        case sol::type::nil:
        case sol::type::none:
            throw sol::error("Cannot pass an object of type nil to RPC");
        case sol::type::string:
            param.set_value(object.as<std::string>());
            break;
        case sol::type::table: {
            sol::table t = object.as<sol::table>();
            if (t.size()) {
                for (std::size_t i = 0; i < t.size(); i++) {
                    set_runtime_parameter(param[i], t[i + 1]);
                }
            } else {
                for (auto &v : t) {
                    set_runtime_parameter(param[v.first.as<std::string>()], v.second);
                }
            }
            break;
        }
        default:
            throw sol::error("Cannot pass an object of unknown type " + std::to_string(static_cast<int>(object.get_type())) + " to RPC");
    }
}

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

struct RPCDevice {
    std::string get_protocol_name() {
        return protocol->type.toStdString();
    }

    sol::object call_rpc_function(const std::string &name, const sol::variadic_args &va) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            throw sol::error("Abort Requested");
        }

        Console::note() << QString("\"%1\" called").arg(name.c_str());
        auto function = protocol->encode_function(name);
        int param_count = 0;
        for (auto &arg : va) {
            auto &param = function.get_parameter(param_count++);
            set_runtime_parameter(param, arg);
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

struct SG04CountDevice {
    std::string get_protocol_name() {
        return protocol->type.toStdString();
    }

    sol::table get_sg04_counts(bool clear) {
        return protocol->get_sg04_counts(*lua, clear);
    }

    sol::state *lua = nullptr;
    SG04CountProtocol *protocol = nullptr;
    CommunicationDevice *device = nullptr;
    ScriptEngine *engine = nullptr;
};

struct ManualDevice {
    std::string get_protocol_name() {
        return protocol->type.toStdString();
    }

    std::string get_name() {
        return protocol->get_name();
    }

    std::string get_manufacturer() {
        return protocol->get_manufacturer();
    }

    std::string get_description() {
        return protocol->get_description();
    }

    std::string get_serial_number() {
        return protocol->get_serial_number();
    }

    std::string get_notes() {
        return protocol->get_notes();
    }

    std::string get_summary() {
        return protocol->get_summary();
    }

    sol::state *lua = nullptr;
    ManualProtocol *protocol = nullptr;
    CommunicationDevice *device = nullptr;
    ScriptEngine *engine = nullptr;
};

struct SCPIDevice {
    void send_command(std::string request) {
        protocol->send_command(request);
    }
    std::string get_protocol_name() {
        return protocol->type.toStdString();
    }

    sol::table get_device_descriptor() {
        sol::table result = lua->create_table_with();
        protocol->get_lua_device_descriptor(result);
        return result;
    }

    sol::table get_str(std::string request) {
        return protocol->get_str(*lua, request); //timeout possible
    }

    sol::table get_str_param(std::string request, std::string argument) {
        return protocol->get_str_param(*lua, request, argument); //timeout possible
    }

    double get_num(std::string request) {
        return protocol->get_num(request); //timeout possible
    }

    double get_num_param(std::string request, std::string argument) {
        return protocol->get_num_param(request, argument); //timeout possible
    }

    bool is_event_received(std::string event_name) {
        return protocol->is_event_received(event_name);
    }

    void clear_event_list() {
        return protocol->clear_event_list();
    }

    sol::table get_event_list() {
        return protocol->get_event_list(*lua);
    }

    std::string get_name(void) {
        return protocol->get_name();
    }

    std::string get_serial_number(void) {
        return protocol->get_serial_number();
    }

    std::string get_manufacturer(void) {
        return protocol->get_manufacturer();
    }

    void set_validation_max_standard_deviation(double max_std_dev) {
        protocol->set_validation_max_standard_deviation(max_std_dev);
    }

    void set_validation_retries(unsigned int retries) {
        protocol->set_validation_retries(retries);
    }

    sol::state *lua = nullptr;
    SCPIProtocol *protocol = nullptr;
    CommunicationDevice *device = nullptr;
    ScriptEngine *engine = nullptr;
};

ScriptEngine::ScriptEngine(UI_container *parent, QPlainTextEdit *console, Data_engine *data_engine)
    : lua(std::make_unique<sol::state>())
    , parent(parent)
    , console(console)
    , data_engine(data_engine) {}

ScriptEngine::~ScriptEngine() {}

std::string ScriptEngine::to_string(double d) {
    if (std::fmod(d, 1.) == 0) {
        return std::to_string(static_cast<int>(d));
    }
    return std::to_string(d);
}

std::string ScriptEngine::to_string(const sol::object &o) {
    switch (o.get_type()) {
        case sol::type::boolean:
            return o.as<bool>() ? "true" : "false";
        case sol::type::function:
            return "function";
        case sol::type::number:
            return to_string(o.as<double>());
        case sol::type::nil:
        case sol::type::none:
            return "nil";
        case sol::type::string:
            return "\"" + o.as<std::string>() + "\"";
        case sol::type::table: {
            auto table = o.as<sol::table>();
            std::string retval{"{"};
            int index = 1;
            for (auto &object : table) {
                auto first_object_string = to_string(object.first);
                if (first_object_string == std::to_string(index++)) {
                    retval += to_string(object.second);
                } else {
                    retval += '[' + std::move(first_object_string) + "]=" + to_string(object.second);
                }
                retval += ", ";
            }
            if (retval.size() > 1) {
                retval.pop_back();
                retval.back() = '}';
                return retval;
            }
            return "{}";
        }
        case sol::type::userdata:
            if (o.is<Color>()) {
                return "Ui.Color(0x" + QString::number(o.as<Color>().rgb & 0xFFFFFFu, 16).toStdString() + ")";
            }
            return "unknown custom datatype";
        default:
            return "unknown type " + std::to_string(static_cast<int>(o.get_type()));
    }
}

std::string ScriptEngine::to_string(const sol::stack_proxy &object) {
    return to_string(sol::object{object});
}

struct Data_engine_handle {
    Data_engine *data_engine{nullptr};
    Data_engine_handle() = delete;
};

void ScriptEngine::load_script(const QString &path) {
    //NOTE: When using lambdas do not capture `this` or by reference, because it breaks when the ScriptEngine is moved
    this->path = path;

    try {
        //load the standard libs if necessary
        lua->open_libraries();

        //add generic function
        {
            (*lua)["show_question"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message, sol::table button_table) {
                return show_question(path, title, message, button_table);
            };

            (*lua)["show_info"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
                show_info(path, title, message);
            };

            (*lua)["show_warning"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
                show_warning(path, title, message);
            };

            (*lua)["print"] = [console = console](const sol::variadic_args &args) {
                if (QThread::currentThread()->isInterruptionRequested()) {
                    throw sol::error("Abort Requested");
                }

                print(console, args);
            };

            (*lua)["sleep_ms"] = [](const unsigned int timeout_ms) { sleep_ms(timeout_ms); };
            (*lua)["pc_speaker_beep"] = []() { pc_speaker_beep(); };
            (*lua)["current_date_time_ms"] = []() { return current_date_time_ms(); };
            (*lua)["round"] = [](const double value, const unsigned int precision = 0) { return round_double(value, precision); };
            (*lua)["require"] = [ path = path, &lua = *lua ](const std::string &file) {
                QDir dir(path);
                dir.cdUp();
                lua.script_file(dir.absoluteFilePath(QString::fromStdString(file) + ".lua").toStdString());
            };
            (*lua)["await_hotkey"] = [] {
                QEventLoop event_loop;
                enum { confirm_pressed, skip_pressed, cancel_pressed };
                std::array<std::unique_ptr<QShortcut>, 3> shortcuts;
                MainWindow::mw->execute_in_gui_thread([&event_loop, &shortcuts] {
                    const char *settings_keys[] = {Globals::confirm_key_sequence, Globals::skip_key_sequence, Globals::cancel_key_sequence};
                    for (std::size_t i = 0; i < shortcuts.size(); i++) {
                        shortcuts[i] =
                            std::make_unique<QShortcut>(QKeySequence::fromString(QSettings{}.value(settings_keys[i], "").toString()), MainWindow::mw);
                        QObject::connect(shortcuts[i].get(), &QShortcut::activated, [&event_loop, i] { event_loop.exit(i); });
                    }
                });
                auto exit_value = event_loop.exec();
                Utility::promised_thread_call(MainWindow::mw, [&shortcuts] { std::fill(std::begin(shortcuts), std::end(shortcuts), nullptr); });
                switch (exit_value) {
                    case confirm_pressed:
                        return "confirm";
                    case skip_pressed:
                        return "skip";
                    case cancel_pressed:
                        return "cancel";
                }
                return "unknown";
            };
        }

        //table functions
        {
            (*lua)["table_save_to_file"] = [console = console](const std::string file_name, sol::table input_table, bool over_write_file) {
                table_save_to_file(console, file_name, input_table, over_write_file);
            };
            (*lua)["table_load_from_file"] = [&lua = *lua, console = console ](const std::string file_name) {
                return table_load_from_file(console, lua, file_name);
            };
            (*lua)["table_sum"] = [](sol::table table) { return table_sum(table); };

            (*lua)["table_mean"] = [](sol::table table) { return table_mean(table); };

            (*lua)["table_set_constant"] = [&lua = *lua](sol::table input_values, double constant) {
                return table_set_constant(lua, input_values, constant);
            };

            (*lua)["table_create_constant"] = [&lua = *lua](const unsigned int size, double constant) {
                return table_create_constant(lua, size, constant);
            };

            (*lua)["table_add_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_add_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_add_constant"] = [&lua = *lua](sol::table input_values, double constant) {
                return table_add_constant(lua, input_values, constant);
            };

            (*lua)["table_sub_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_sub_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_mul_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_mul_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_mul_constant"] = [&lua = *lua](sol::table input_values_a, double constant) {
                return table_mul_constant(lua, input_values_a, constant);
            };

            (*lua)["table_div_table"] = [&lua = *lua](sol::table input_values_a, sol::table input_values_b) {
                return table_div_table(lua, input_values_a, input_values_b);
            };

            (*lua)["table_round"] = [&lua = *lua](sol::table input_values, const unsigned int precision = 0) {
                return table_round(lua, input_values, precision);
            };

            (*lua)["table_abs"] = [&lua = *lua](sol::table input_values) {
                return table_abs(lua, input_values);
            };

            (*lua)["table_mid"] = [&lua = *lua](sol::table input_values, const unsigned int start, const unsigned int length) {
                return table_mid(lua, input_values, start, length);
            };

            (*lua)["table_equal_constant"] = [](sol::table input_values_a, double input_const_val) {
                return table_equal_constant(input_values_a, input_const_val);
            };

            (*lua)["table_equal_table"] = [](sol::table input_values_a, sol::table input_values_b) {
                return table_equal_table(input_values_a, input_values_b);
            };

            (*lua)["table_max"] = [](sol::table input_values) { return table_max(input_values); };

            (*lua)["table_min"] = [](sol::table input_values) { return table_min(input_values); };

            (*lua)["table_max_abs"] = [](sol::table input_values) { return table_max_abs(input_values); };

            (*lua)["table_min_abs"] = [](sol::table input_values) { return table_min_abs(input_values); };
            (*lua)["git_info"] = [&lua = *lua](std::string path, bool allow_modified) {
                return git_info (lua, path, allow_modified);
            };
        }

        {
            (*lua)["measure_noise_level_czt"] = [&lua = *lua](sol::table rpc_device, const unsigned int dacs_quantity,
                                                              const unsigned int max_possible_dac_value) {
                return measure_noise_level_czt(lua, rpc_device, dacs_quantity, max_possible_dac_value);
            };
        }
        //bind DataLogger
        {
            lua->new_usertype<DataLogger>(
                "DataLogger", //
                sol::meta_function::construct,
                [console = console](const std::string &file_name, char seperating_character, sol::table field_names, bool over_write_file) {
                    return DataLogger{console, file_name, seperating_character, field_names, over_write_file};
                }, //

                "append_data",
                [](DataLogger &handle, const sol::table &data_record) { return handle.append_data(data_record); }, //
                "save", [](DataLogger &handle) { handle.save(); }                                                  //
                );
        }
        //bind charge counter
        {
            lua->new_usertype<ChargeCounter>("ChargeCounter",                                                                  //
                                             sol::meta_function::construct, [console = console]() { return ChargeCounter{}; }, //

                                             "add_current", [](ChargeCounter &handle, const double current) { return handle.add_current(current); }, //
                                             "reset", [](ChargeCounter &handle, const double current) { handle.reset(); },                           //
                                             "get_current_hours", [](ChargeCounter &handle) { return handle.get_current_hours(); }                   //
                                             );
        }
        //bind data engine
        {
            lua->new_usertype<Data_engine_handle>(
                "Data_engine", //
                sol::meta_function::construct, [ data_engine = data_engine, pdf_filepath = pdf_filepath.get(), form_filepath = form_filepath.get() ](
                                                   const std::string &xml_file, const std::string &json_file, const sol::table &dependency_tags) {
                    QString form_dir = QSettings{}.value(Globals::form_directory, QDir::currentPath()).toString();
                    auto file_path = QDir{form_dir}.absoluteFilePath(QString::fromStdString(json_file)).toStdString();
                    std::ifstream f(file_path);
                    if (!f) {
                        throw std::runtime_error("Failed opening file " + file_path);
                    }

                    auto add_value_to_tag_list = [](QList<QVariant> &values, const sol::object &obj, const std::string &tag_name) {
                        if (obj.get_type() == sol::type::string) {
                            values.append(QString().fromStdString(obj.as<std::string>()));
                        } else if (obj.get_type() == sol::type::number) {
                            QVariant v;
                            v.setValue<double>(obj.as<double>());
                            values.append(v);
                        } else if (obj.get_type() == sol::type::boolean) {
                            QVariant v;
                            v.setValue<bool>(obj.as<bool>());
                            values.append(v);
                        } else {
                            throw std::runtime_error(
                                QString("invalid type in field of dependency tags at index %1").arg(QString().fromStdString(tag_name)).toStdString());
                        }
                    };

                    QMap<QString, QList<QVariant>> tags;
                    for (auto &tag : dependency_tags) {
                        std::string tag_name = tag.first.as<std::string>();
                        QList<QVariant> values;

                        if (tag.second.get_type() == sol::type::table) {
                            const auto &value_list = tag.second.as<sol::table>();
                            for (int i = 1; i <= value_list.size(); i++) {
                                const sol::object &obj = value_list[i].get<sol::object>();
                                add_value_to_tag_list(values, obj, tag_name);
                            }
                        } else {
                            add_value_to_tag_list(values, tag.second, tag_name);
                        }

                        tags.insert(QString().fromStdString(tag_name), values);
                    }
                    data_engine->set_dependancy_tags(tags);
                    data_engine->set_source(f);
                    *pdf_filepath = QDir{QSettings{}.value(Globals::form_directory, "").toString()}.absoluteFilePath("test_dump.pdf").toStdString();
                    *form_filepath =
                        QDir{QSettings{}.value(Globals::form_directory, "").toString()}.absoluteFilePath(QString::fromStdString(xml_file)).toStdString();

                    return Data_engine_handle{data_engine};
                }, //
                "use_instance",
                [](Data_engine_handle &handle, const std::string &section_name, const std::string &instance_caption, const uint instance_index) {
                    handle.data_engine->use_instance(QString::fromStdString(section_name), QString::fromStdString(instance_caption), instance_index);
                },
                "get_instance_count",
                [](Data_engine_handle &handle, const std::string &section_name) { return handle.data_engine->get_instance_count(section_name); },
                "get_description",
                [](Data_engine_handle &handle, const std::string &id) { return handle.data_engine->get_description(QString::fromStdString(id)).toStdString(); },
                "get_actual_value", [](Data_engine_handle &handle,
                                       const std::string &id) { return handle.data_engine->get_actual_value(QString::fromStdString(id)).toStdString(); },
                "get_unit",
                [](Data_engine_handle &handle, const std::string &id) { return handle.data_engine->get_unit(QString::fromStdString(id)).toStdString(); },
                "get_desired_value",
                [](Data_engine_handle &handle, const std::string &id) {
                    return handle.data_engine->get_desired_value_as_string(QString::fromStdString(id)).toStdString();
                },
                "get_section_names", [&lua = *lua](Data_engine_handle & handle) { return handle.data_engine->get_section_names(&lua); }, //
                "get_ids_of_section", [&lua = *lua](Data_engine_handle & handle,
                                                    const std::string &section_name) { return handle.data_engine->get_ids_of_section(&lua, section_name); },
                "set_instance_count",
                [](Data_engine_handle &handle, const std::string &instance_count_name, const uint instance_count) {
                    handle.data_engine->set_instance_count(QString::fromStdString(instance_count_name), instance_count);
                },
                "value_in_range",
                [](Data_engine_handle &handle, const std::string &field_id) { return handle.data_engine->value_in_range(QString::fromStdString(field_id)); },
                "is_bool",
                [](Data_engine_handle &handle, const std::string &field_id) { return handle.data_engine->is_bool(QString::fromStdString(field_id)); },
                "is_text",
                [](Data_engine_handle &handle, const std::string &field_id) { return handle.data_engine->is_text(QString::fromStdString(field_id)); },
                "is_number",
                [](Data_engine_handle &handle, const std::string &field_id) { return handle.data_engine->is_number(QString::fromStdString(field_id)); },
                "set_actual_number", [](Data_engine_handle &handle, const std::string &field_id,
                                        double value) { handle.data_engine->set_actual_number(QString::fromStdString(field_id), value); },
                "set_actual_bool", [](Data_engine_handle &handle, const std::string &field_id,
                                      bool value) { handle.data_engine->set_actual_bool(QString::fromStdString(field_id), value); },

                "set_actual_text",
                [](Data_engine_handle &handle, const std::string &field_id, const std::string text) {
                    handle.data_engine->set_actual_text(QString::fromStdString(field_id), QString::fromStdString(text));
                });
        }

        //bind UI
        auto ui_table = lua->create_named_table("Ui");

        //UI functions
        {
            ui_table["set_column_count"] = [container = parent](int count) {
                Utility::thread_call(MainWindow::mw, [container, count] { container->set_column_count(count); });
            };
        }

        //bind plot
        {
            ui_table.new_usertype<Lua_UI_Wrapper<Curve>>(
                "Curve",                                                               //
                sol::meta_function::construct, sol::no_constructor,                    //
                "append_point", thread_call_wrapper_non_waiting(&Curve::append_point), //
                "add_spectrum",
                [](Lua_UI_Wrapper<Curve> &curve, sol::table table) {
                    std::vector<double> data;
                    data.reserve(table.size());
                    for (auto &i : table) {
                        data.push_back(i.second.as<double>());
                    }
                    Utility::thread_call(MainWindow::mw, [ id = curve.id, data = std::move(data) ] {
                        auto &curve = MainWindow::mw->get_lua_UI_class<Curve>(id);
                        curve.add(data);
                    });
                }, //
                "add_spectrum_at",
                [](Lua_UI_Wrapper<Curve> &curve, const unsigned int spectrum_start_channel, const sol::table &table) {
                    std::vector<double> data;
                    data.reserve(table.size());
                    for (auto &i : table) {
                        data.push_back(i.second.as<double>());
                    }
                    Utility::thread_call(MainWindow::mw, [ id = curve.id, data = std::move(data), spectrum_start_channel ] {
                        auto &curve = MainWindow::mw->get_lua_UI_class<Curve>(id);
                        curve.add_spectrum_at(spectrum_start_channel, data);
                    });
                }, //

                "clear",
                thread_call_wrapper(&Curve::clear),                                            //
                "set_median_enable", thread_call_wrapper(&Curve::set_median_enable),           //
                "set_median_kernel_size", thread_call_wrapper(&Curve::set_median_kernel_size), //
                "integrate_ci", thread_call_wrapper(&Curve::integrate_ci),                     //
                "set_x_axis_gain", thread_call_wrapper(&Curve::set_x_axis_gain),               //
                "set_x_axis_offset",
                thread_call_wrapper(&Curve::set_x_axis_offset),      //
                "set_color", thread_call_wrapper(&Curve::set_color), //
                "user_pick_x_coord",
                [](const Lua_UI_Wrapper<Curve> &lua_curve) {
                    QThread *thread = QThread::currentThread();
                    std::promise<double> x_selection_promise;
                    std::future<double> x_selection_future = x_selection_promise.get_future();
                    Utility::thread_call(MainWindow::mw, [&lua_curve, thread, x_selection_promise = &x_selection_promise ]() mutable {
                        Curve &curve = MainWindow::mw->get_lua_UI_class<Curve>(lua_curve.id);
                        curve.set_onetime_click_callback([thread, x_selection_promise](double x, double y) mutable {
                            (void)y;
                            x_selection_promise->set_value(x);
                            Utility::thread_call(thread, [thread] { thread->exit(1234); });
                        });
                    });
                    if (QEventLoop{}.exec() == 1234) {
                        return x_selection_future.get();
                    } else {
                        throw sol::error("aborted");
                    }
                }
                //
                );
            ui_table.new_usertype<Lua_UI_Wrapper<Plot>>("Plot",                                                                                          //
                                                        sol::meta_function::construct, [parent = this->parent] { return Lua_UI_Wrapper<Plot>{parent}; }, //
                                                        "clear",
                                                        thread_call_wrapper(&Plot::clear), //
                                                        "add_curve",
                                                        [parent = this->parent](Lua_UI_Wrapper<Plot> & lua_plot)->Lua_UI_Wrapper<Curve> {
                                                            return Utility::promised_thread_call(MainWindow::mw,
                                                                                                 [parent, &lua_plot] {
                                                                                                     auto &plot =
                                                                                                         MainWindow::mw->get_lua_UI_class<Plot>(lua_plot.id);
                                                                                                     return Lua_UI_Wrapper<Curve>{parent, &plot};
                                                                                                 } //
                                                                                                 );
                                                        }, //
                                                        "set_x_marker",
                                                        thread_call_wrapper(&Plot::set_x_marker), //
                                                        "set_visible", thread_call_wrapper(&Plot::set_visible));
        }
        //bind color
        {
            ui_table.new_usertype<Color>("Color", //
                                         sol::meta_function::construct, sol::no_constructor);
            ui_table["Color"] = overloaded_function([](const std::string &name) { return Color{name}; },
                                                    [](int r, int g, int b) {
                                                        return Color{r, g, b};
                                                    }, //
                                                    [](int rgb) { return Color{rgb}; });
        }
        //bind DataEngineInput
        {
            ui_table.new_usertype<Lua_UI_Wrapper<DataEngineInput>>(
                "DataEngineInput", //
                sol::meta_function::construct,
                [parent = this->parent](Data_engine_handle & handle, const std::string &field_id, const std::string &extra_explanation,
                                        const std::string &empty_value_placeholder, const std::string &actual_prefix, const std::string &desired_prefix) {
                    return Lua_UI_Wrapper<DataEngineInput>{parent,        handle.data_engine, field_id, extra_explanation, empty_value_placeholder,
                                                           actual_prefix, desired_prefix};
                }, //
                "load_actual_value",
                thread_call_wrapper(&DataEngineInput::load_actual_value), //
                "await_event",
                thread_call_wrapper(&DataEngineInput::await_event),                //
                "set_visible", thread_call_wrapper(&DataEngineInput::set_visible), //
                "set_enabled", thread_call_wrapper(&DataEngineInput::set_enabled), //
                "save_to_data_engine",
                thread_call_wrapper(&DataEngineInput::save_to_data_engine), //
                "set_editable", thread_call_wrapper(&DataEngineInput::set_editable)

                    );
        }
        //bind ComboBoxFileSelector
        {
            ui_table.new_usertype<Lua_UI_Wrapper<ComboBoxFileSelector>>("ComboBoxFileSelector", //
                                                                        sol::meta_function::construct,
                                                                        [parent = this->parent](const std::string &directory, const sol::table &filters) {
                                                                            return Lua_UI_Wrapper<ComboBoxFileSelector>{parent, directory, filters};
                                                                        }, //
                                                                        "get_selected_file",
                                                                        thread_call_wrapper(&ComboBoxFileSelector::get_selected_file), //
                                                                        "set_visible",
                                                                        thread_call_wrapper(&ComboBoxFileSelector::set_visible), //
                                                                        "set_order_by",
                                                                        thread_call_wrapper(&ComboBoxFileSelector::set_order_by) //

                                                                        );
        }
        //bind IsotopeSourceSelector
        {
            ui_table.new_usertype<Lua_UI_Wrapper<IsotopeSourceSelector>>(
                "IsotopeSourceSelector",                                                                                            //
                sol::meta_function::construct, [parent = this->parent]() { return Lua_UI_Wrapper<IsotopeSourceSelector>{parent}; }, //
                "set_visible",
                thread_call_wrapper(&IsotopeSourceSelector::set_visible), //
                "get_selected_activity_Bq",
                thread_call_wrapper(&IsotopeSourceSelector::get_selected_activity_Bq),                                //
                "get_selected_serial_number", thread_call_wrapper(&IsotopeSourceSelector::get_selected_serial_number) //
                );
        }
        //bind ComboBox
        {
            ui_table.new_usertype<Lua_UI_Wrapper<ComboBox>>("ComboBox", //
                                                            sol::meta_function::construct,
                                                            [parent = this->parent](sol::table items) {
                                                                return Lua_UI_Wrapper<ComboBox>{parent, items};
                                                            }, //
                                                            "set_items",
                                                            thread_call_wrapper(&ComboBox::set_items), //
                                                            "get_text",
                                                            thread_call_wrapper(&ComboBox::get_text), //
                                                            "set_index",
                                                            thread_call_wrapper(&ComboBox::set_index), //
                                                            "get_index",
                                                            thread_call_wrapper(&ComboBox::get_index), //
                                                            "set_visible",
                                                            thread_call_wrapper(&ComboBox::set_visible), //
                                                            "set_caption",
                                                            thread_call_wrapper(&ComboBox::set_caption), //
                                                            "get_caption",
                                                            thread_call_wrapper(&ComboBox::get_caption) //
                                                            );
        }
        //bind SpinBox
        {
            ui_table.new_usertype<Lua_UI_Wrapper<SpinBox>>("SpinBox", //
                                                           sol::meta_function::construct,
                                                           [parent = this->parent]() { return Lua_UI_Wrapper<SpinBox>{parent}; }, //
                                                           "get_value",
                                                           thread_call_wrapper(&SpinBox::get_value), //
                                                           "set_max_value",
                                                           thread_call_wrapper(&SpinBox::set_max_value), //
                                                           "set_min_value",
                                                           thread_call_wrapper(&SpinBox::set_min_value), //
                                                           "set_value",
                                                           thread_call_wrapper(&SpinBox::set_value), //
                                                           "set_visible",
                                                           thread_call_wrapper(&SpinBox::set_visible), //
                                                           "set_caption",
                                                           thread_call_wrapper(&SpinBox::set_caption), //
                                                           "get_caption",
                                                           thread_call_wrapper(&SpinBox::get_caption) //
                                                           );
        }
        //bind ProgressBar
        {
            ui_table.new_usertype<Lua_UI_Wrapper<ProgressBar>>("ProgressBar", //
                                                               sol::meta_function::construct,
                                                               [parent = this->parent]() { return Lua_UI_Wrapper<ProgressBar>{parent}; }, //
                                                               "set_max_value",
                                                               thread_call_wrapper(&ProgressBar::set_max_value), //
                                                               "set_min_value",
                                                               thread_call_wrapper(&ProgressBar::set_min_value), //
                                                               "set_value",
                                                               thread_call_wrapper(&ProgressBar::set_value), //
                                                               "increment_value",
                                                               thread_call_wrapper(&ProgressBar::increment_value), //
                                                               "set_visible",
                                                               thread_call_wrapper(&ProgressBar::set_visible), //
                                                               "set_caption",
                                                               thread_call_wrapper(&ProgressBar::set_caption), //
                                                               "get_caption",
                                                               thread_call_wrapper(&ProgressBar::get_caption) //
                                                               );
        }
        //bind Label
        {
            ui_table.new_usertype<Lua_UI_Wrapper<Label>>("Label", //
                                                         sol::meta_function::construct,
                                                         [parent = this->parent](const std::string &text) {
                                                             return Lua_UI_Wrapper<Label>{parent, text};
                                                         }, //
                                                         "set_text",
                                                         thread_call_wrapper(&Label::set_text), //
                                                         "set_visible",
                                                         thread_call_wrapper(&Label::set_visible), //
                                                         "set_font_size",
                                                         thread_call_wrapper(&Label::set_font_size), //
                                                         "get_text", thread_call_wrapper(&Label::get_text));
        }
        //bind hline
        {
            ui_table.new_usertype<Lua_UI_Wrapper<HLine>>("HLine",                                                                                            //
                                                         sol::meta_function::construct, [parent = this->parent]() { return Lua_UI_Wrapper<HLine>{parent}; }, //

                                                         "set_visible", thread_call_wrapper(&HLine::set_visible));
        }
        //bind CheckBox
        {
            ui_table.new_usertype<Lua_UI_Wrapper<CheckBox>>("CheckBox", //
                                                            sol::meta_function::construct,
                                                            [parent = this->parent](const std::string &text) {
                                                                return Lua_UI_Wrapper<CheckBox>{parent, text};
                                                            }, //
                                                            "set_checked",
                                                            thread_call_wrapper(&CheckBox::set_checked), //
                                                            "get_checked",
                                                            thread_call_wrapper(&CheckBox::get_checked), //
                                                            "set_visible",
                                                            thread_call_wrapper(&CheckBox::set_visible), //
                                                            "set_text",
                                                            thread_call_wrapper(&CheckBox::set_text), //
                                                            "get_text", thread_call_wrapper(&CheckBox::get_text));
        }
        //bind Image
        {
            ui_table.new_usertype<Lua_UI_Wrapper<Image>>("Image",                                                                                            //
                                                         sol::meta_function::construct, [parent = this->parent]() { return Lua_UI_Wrapper<Image>{parent}; }, //
                                                         "load_image_file", thread_call_wrapper(&Image::load_image_file),                                    //
                                                         "set_visible",
                                                         thread_call_wrapper(&Image::set_visible) //
                                                         );
        }
        //bind button
        {
            ui_table.new_usertype<Lua_UI_Wrapper<Button>>(
                "Button", //
                sol::meta_function::construct,
                [parent = this->parent](const std::string &title) {
                    return Lua_UI_Wrapper<Button>{parent, title};
                }, //
                "has_been_clicked",
                thread_call_wrapper(&Button::has_been_clicked), //
                "set_visible",
                thread_call_wrapper(&Button::set_visible), //
                "await_click",
                [](const Lua_UI_Wrapper<Button> &lew) {
                    auto &lb = MainWindow::mw->get_lua_UI_class<Button>(lew.id);
                    lb.set_single_shot_return_pressed_callback([thread = QThread::currentThread()] { thread->exit(); });
                    QEventLoop{}.exec();
                } //
                );
        }

        //bind edit field
        {
            ui_table.new_usertype<Lua_UI_Wrapper<LineEdit>>(
                "LineEdit",                                                                                          //
                sol::meta_function::construct, [parent = this->parent] { return Lua_UI_Wrapper<LineEdit>(parent); }, //
                "set_placeholder_text", thread_call_wrapper(&LineEdit::set_placeholder_text),                        //
                "get_text", thread_call_wrapper(&LineEdit::get_text),                                                //
                "set_text", thread_call_wrapper(&LineEdit::set_text),                                                //
                "set_name", thread_call_wrapper(&LineEdit::set_name),                                                //
                "get_name", thread_call_wrapper(&LineEdit::get_name),                                                //
                "get_number", thread_call_wrapper(&LineEdit::get_number),                                            //
                "get_caption", thread_call_wrapper(&LineEdit::get_caption),                                          //
                "set_caption", thread_call_wrapper(&LineEdit::set_caption),                                          //
                "set_visible",
                thread_call_wrapper(&LineEdit::set_visible), //
                "await_return",
                [](const Lua_UI_Wrapper<LineEdit> &lew) {
                    auto le = MainWindow::mw->get_lua_UI_class<LineEdit>(lew.id);
                    le.set_single_shot_return_pressed_callback([thread = QThread::currentThread()] { thread->exit(); });
                    QEventLoop{}.exec();
                    auto text = Utility::promised_thread_call(MainWindow::mw, [&le] { return le.get_text(); });
                    return text;
                } //
                );
        }
        {
            lua->new_usertype<SCPIDevice>(
                "SCPIDevice",                                                                                                                               //
                sol::meta_function::construct, sol::no_constructor,                                                                                         //
                "get_protocol_name", [](SCPIDevice &protocol) { return protocol.get_protocol_name(); },                                                     //
                "get_device_descriptor", [](SCPIDevice &protocol) { return protocol.get_device_descriptor(); },                                             //
                "get_str", [](SCPIDevice &protocol, std::string request) { return protocol.get_str(request); },                                             //
                "get_str_param", [](SCPIDevice &protocol, std::string request, std::string argument) { return protocol.get_str_param(request, argument); }, //
                "get_num", [](SCPIDevice &protocol, std::string request) { return protocol.get_num(request); },                                             //
                "get_num_param", [](SCPIDevice &protocol, std::string request, std::string argument) { return protocol.get_num_param(request, argument); }, //
                "get_name", [](SCPIDevice &protocol) { return protocol.get_name(); },                                                                       //
                "get_serial_number", [](SCPIDevice &protocol) { return protocol.get_serial_number(); },                                                     //
                "get_manufacturer", [](SCPIDevice &protocol) { return protocol.get_manufacturer(); },                                                       //
                "is_event_received", [](SCPIDevice &protocol, std::string event_name) { return protocol.is_event_received(event_name); },                   //
                "clear_event_list", [](SCPIDevice &protocol) { return protocol.clear_event_list(); },                                                       //
                "get_event_list", [](SCPIDevice &protocol) { return protocol.get_event_list(); },                                                           //
                "set_validation_max_standard_deviation",
                [](SCPIDevice &protocoll, double max_std_dev) { return protocoll.set_validation_max_standard_deviation(max_std_dev); },          //
                "set_validation_retries", [](SCPIDevice &protocoll, unsigned int retries) { return protocoll.set_validation_retries(retries); }, //
                "send_command", [](SCPIDevice &protocoll, std::string request) { return protocoll.send_command(request); }                       //

                );
        }

        {
            lua->new_usertype<SG04CountDevice>("SG04CountDevice",                                                                           //
                                               sol::meta_function::construct, sol::no_constructor,                                          //
                                               "get_protocol_name", [](SG04CountDevice &protocol) { return protocol.get_protocol_name(); }, //
                                               "get_sg04_counts",
                                               [](SG04CountDevice &protocol, bool clear_on_read) { return protocol.get_sg04_counts(clear_on_read); } //

                                               );
        }
        {
            lua->new_usertype<ManualDevice>("ManualDevice",                                                                           //
                                            sol::meta_function::construct, sol::no_constructor,                                       //
                                            "get_protocol_name", [](ManualDevice &protocol) { return protocol.get_protocol_name(); }, //
                                            "get_name", [](ManualDevice &protocol) { return protocol.get_name(); },                   //
                                            "get_manufacturer", [](ManualDevice &protocol) { return protocol.get_manufacturer(); },   //
                                            "get_description", [](ManualDevice &protocol) { return protocol.get_description(); },     //
                                            "get_serial_number", [](ManualDevice &protocol) { return protocol.get_serial_number(); }, //
                                            "get_notes", [](ManualDevice &protocol) { return protocol.get_notes(); },                 //
                                            "get_summary", [](ManualDevice &protocol) { return protocol.get_summary(); }              //

                                            );
        }
        lua->script_file(path.toStdString());
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
    return lua->create_table_with();
}

QStringList ScriptEngine::get_string_list(const QString &name) {
    QStringList retval;
    sol::table t = lua->get<sol::table>(name.toStdString());
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
int get_quantity_num(sol::object &obj) {
    int result = 0;
    if (obj.get_type() == sol::type::string) {
        std::string str = obj.as<std::string>();
        QString qstr = QString().fromStdString(str);
        bool ok = false;

        result = qstr.toInt(&ok);
        if (ok == false) {
            result = INT_MAX;
        }
    } else if (obj.get_type() == sol::type::number) {
        result = obj.as<int>();
    }
    return result;
}

std::vector<DeviceRequirements> ScriptEngine::get_device_requirement_list(const QString &name) {
    std::vector<DeviceRequirements> result{};
    sol::table protocol_entries = lua->get<sol::table>(name.toStdString());
    try {
        if (protocol_entries.valid() == false) {
            return result;
        }
        for (auto &protocol_entry : protocol_entries) {
            DeviceRequirements item{};
            sol::table protocol_entry_table = protocol_entry.second.as<sol::table>();
            bool device_name_found = false;
            bool protocol_does_not_provide_name = false;
            for (auto &protocol_entry_field : protocol_entry_table) {
                if (protocol_entry_field.first.as<std::string>() == "protocol") {
                    item.protocol_name = QString().fromStdString(protocol_entry_field.second.as<std::string>());
                    if (item.protocol_name == "SG04Count") {
                        protocol_does_not_provide_name = true;
                    }
                } else if (protocol_entry_field.first.as<std::string>() == "device_names") {
                    device_name_found = true;
                    if (protocol_entry_field.second.get_type() == sol::type::table) {
                        sol::table device_names = protocol_entry_field.second.as<sol::table>();
                        for (auto &device_name : device_names) {
                            std::string str = device_name.second.as<std::string>();
                            if (str == "") {
                                str = "*";
                            }
                            item.device_names.append(QString().fromStdString(str));
                        }
                    }
                    if (protocol_entry_field.second.get_type() == sol::type::string) {
                        std::string str = protocol_entry_field.second.as<std::string>();
                        if (str == "") {
                            str = "*";
                        }
                        item.device_names.append(QString().fromStdString(str));
                    }
                } else if (protocol_entry_field.first.as<std::string>() == "quantity") {
                    item.quantity_min = get_quantity_num(protocol_entry_field.second);
                    item.quantity_max = item.quantity_min;
                } else if (protocol_entry_field.first.as<std::string>() == "quantity_min") {
                    item.quantity_min = get_quantity_num(protocol_entry_field.second);
                } else if (protocol_entry_field.first.as<std::string>() == "quantity_max") {
                    item.quantity_max = get_quantity_num(protocol_entry_field.second);
                }
            }
            if (protocol_does_not_provide_name) {
                device_name_found = false;
                item.device_names.clear();
            }
            if (!device_name_found) {
                item.device_names.append("*");
            }
            result.push_back(item);
        }
    } catch (const sol::error &error) {
        set_error(error);
        throw;
    }
    return result;
}

void ScriptEngine::run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices) {
    auto reset_lua_state = [this] {
        lua = std::make_unique<sol::state>();
        if (pdf_filepath->empty() == false) {
            data_engine->generate_pdf(*form_filepath, *pdf_filepath);
        }
    };
    try {
        {
            auto device_list = lua->create_table_with();
            for (auto &device_protocol : devices) {
                if (auto rpcp = dynamic_cast<RPCProtocol *>(device_protocol.second)) {
                    device_list.add(RPCDevice{&*lua, rpcp, device_protocol.first, this});

                    auto type_reg = lua->create_simple_usertype<RPCDevice>();
                    for (auto &function : rpcp->get_description().get_functions()) {
                        const auto &function_name = function.get_function_name();
                        type_reg.set(function_name,
                                     [function_name](RPCDevice &device, const sol::variadic_args &va) { return device.call_rpc_function(function_name, va); });
                        add_enum_types(function, *lua);
                    }
                    type_reg.set("get_protocol_name", [](RPCDevice &device) { return device.get_protocol_name(); });
                    const auto &type_name = "RPCDevice_" + rpcp->get_description().get_hash();
                    lua->set_usertype(type_name, type_reg);
                    while (device_protocol.first->waitReceived(CommunicationDevice::Duration{0}, 1)) {
                        //ignore leftover data in the receive buffer
                    }
                    rpcp->clear();

                } else if (auto scpip = dynamic_cast<SCPIProtocol *>(device_protocol.second)) {
                    device_list.add(SCPIDevice{&*lua, scpip, device_protocol.first, this});
                    scpip->clear();
                } else if (auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(device_protocol.second)) {
                    device_list.add(SG04CountDevice{&*lua, sg04_count_protocol, device_protocol.first, this});
                } else if (auto manual_protocol = dynamic_cast<ManualProtocol *>(device_protocol.second)) {
                    device_list.add(ManualDevice{&*lua, manual_protocol, device_protocol.first, this});
                } else {
                    //TODO: other protocols
                    throw std::runtime_error("invalid protocol: " + device_protocol.second->type.toStdString());
                }
            }
            (*lua)["run"](device_list);
        }
        reset_lua_state();
    } catch (const sol::error &e) {
        set_error(e);
        reset_lua_state();
        throw;
    }
}

QString DeviceRequirements::get_description() const {
    QString quantity;
    if (quantity_max == INT_MAX) {
        quantity = "(" + QString::number(quantity_min) + "+)";
    } else if (quantity_min == quantity_max) {
        quantity = "(" + QString::number(quantity_min) + ")";
    } else {
        quantity = "(" + QString::number(quantity_min) + "-" + QString::number(quantity_max) + ")";
    }
    return device_names.join("/") + " " + quantity;
}
