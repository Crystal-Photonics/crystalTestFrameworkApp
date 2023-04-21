#include "scriptengine.h"
#include "LuaFunctions/chargecounter.h"
#include "LuaFunctions/color.h"
#include "LuaFunctions/dataengineinput_lua.h"
#include "LuaFunctions/datalogger.h"
#include "LuaFunctions/lua_functions.h"
#include "Protocols/rpcprotocol.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "communication_devices.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h"
#include "data_engine/exceptionalapproval.h"
#include "qt_util.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_function.h"
#include "scriptsetup.h"
#include "testrunner.h"
#include "ui_container.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#include <QMutex>
#include <QPlainTextEdit>
#include <QProcess>
#include <QSettings>
#include <QShortcut>
#include <QThread>
#include <QVariant>
#include <cmath>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <vector>

void interrupt_script_engine(ScriptEngine *script_engine, QString message) {
    script_engine->post_interrupt(message);
}

QString get_absolute_file_path(const QString &script_path, const QString &file_to_open) {
    QDir dir(file_to_open);
    QString result;
    if (dir.isRelative()) {
        QDir base(QFileInfo(script_path).absoluteDir());
        result = base.absoluteFilePath(file_to_open);
    } else {
        result = file_to_open;
    }
    create_path(result);
    return result;
}

std::string get_absolute_file_path(const QString &script_path, const std::string &file_to_open) {
    return get_absolute_file_path(script_path, QString::fromStdString(file_to_open)).toStdString();
}

static sol::object create_lua_object_from_RPC_answer(const RPCRuntimeDecodedParam &param, sol::state &lua) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            auto array = param.as_array();
            if (array.front().get_desciption()->get_type() == RPCRuntimeParameterDescription::Type::character) {
                std::string result_string = param.as_string();
                return sol::make_object(lua.lua_state(), result_string);
            } else {
                if (array.size() == 1) {
                    return create_lua_object_from_RPC_answer(array.front(), lua);
                }
                auto table = lua.create_table_with();
                for (auto &element : array) {
                    table.add(create_lua_object_from_RPC_answer(element, lua));
                }
                return table;
            }
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

struct RPCDevice {
    std::string get_protocol_name() {
        return protocol->type.toStdString();
    }
    bool is_protocol_device_available() {
#if 1
        auto result = protocol->call_get_hash_function(0);
        if (result.decoded_function_call_reply) {
            return true;
        }
#endif
        return false;
    }

    sol::object call_rpc_function(const std::string &name, const sol::variadic_args &va, bool show_message_box_when_timeout) {
        Console_handle::note() << QString("\"%1\" called").arg(name.c_str());
        auto function = protocol->encode_function(name);
        int param_count = 0;
        for (auto &arg : va) {
            auto &param = function.get_parameter(param_count++);
            set_runtime_parameter(param, arg);
        }
        if (not function.are_all_values_set()) {
            throw sol::error("Failed calling function, missing parameters");
        }
        auto result = protocol->call_and_wait(function, show_message_box_when_timeout);
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
                Console_handle::error() << Sol_error_message{e.what(), engine->runner->get_name(), engine->runner->get_script_path()};
                throw;
            }
        }
        throw sol::error("Call to \"" + name + "\" failed");
    }
    bool has_function(const std::string &name) const {
        return protocol->has_function(name);
    }

    sol::state *lua = nullptr;
    RPCProtocol *protocol = nullptr;
    CommunicationDevice *device = nullptr;
    ScriptEngine *engine = nullptr;
    sol::table enums;
};

static void add_enum_type(const RPCRuntimeParameterDescription &param, sol::state &lua, sol::table &device) {
    if (param.get_type() == RPCRuntimeParameterDescription::Type::enumeration) {
        const auto &enum_description = param.as_enumeration();
        auto table = lua.create_table_with();
        for (auto &value : enum_description.values) {
            table[value.name] = value.to_int();
        }
        table["to_string"] = [enum_description](int enum_value_param) -> std::string {
            for (const auto &enum_value : enum_description.values) {
                if (enum_value.to_int() == enum_value_param) {
                    return enum_value.name;
                }
            }
            return "";
        };
        device[enum_description.enum_name] = std::move(table);
    } else if (param.get_type() == RPCRuntimeParameterDescription::Type::array) {
        auto array = param.as_array();

        if ((array.type.get_type() == RPCRuntimeParameterDescription::Type::enumeration)) {
            //&& (array.number_of_elements == 1)
            add_enum_type(array.type, lua, device);
        } else if ((array.type.get_type() == RPCRuntimeParameterDescription::Type::structure)) {
            auto structure = array.type.as_structure();
            for (auto &member : structure.members) {
                add_enum_type(member, lua, device);
            }
        }
    }
}

static void add_enum_types(const RPCRuntimeFunction &function, sol::state &lua, sol::table &device) {
    for (auto &param : function.get_request_parameters()) {
        add_enum_type(param, lua, device);
    }
    for (auto &param : function.get_reply_parameters()) {
        add_enum_type(param, lua, device);
    }
}

static void abort_check() {
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw sol::error("Abort Requested");
    }
}

ScriptEngine::ScriptEngine(UI_container *parent, Console &console, TestRunner *runner, QString test_name)
    : runner{runner}
    , test_name{std::move(test_name)}
    , parent{parent}
    , console(console) {
    reset_lua_state();
}

ScriptEngine::~ScriptEngine() { //
    lua_devices = std::nullopt;
}

Event_id::Event_id ScriptEngine::await_timeout(std::chrono::milliseconds duration, std::chrono::milliseconds start) {
    std::unique_lock<std::mutex> lock{await_mutex};
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    await_condition_variable.wait_for(lock, duration - start, [this] { return await_condition == Event_id::interrupted; });
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    return Event_id::Timer_expired;
}

Event_id::Event_id ScriptEngine::await_ui_event() {
    std::unique_lock<std::mutex> lock{await_mutex};
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    await_condition_variable.wait(lock, [this] { return await_condition == Event_id::interrupted || await_condition == Event_id::UI_activated; });
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    await_condition = Event_id::invalid;
    return Event_id::Timer_expired;
}

Event_id::Event_id ScriptEngine::await_hotkey_event() {
    std::unique_lock<std::mutex> lock{await_mutex};
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    // qDebug() << "eventloop Interruption";
    auto connections = Utility::promised_thread_call(MainWindow::mw, [this] {
        std::array<QMetaObject::Connection, 3> connections;
        std::array<void (UI_container::*)(), 3> key_signals = {&UI_container::confirm_pressed, &UI_container::cancel_pressed, &UI_container::skip_pressed};
        std::array<Event_id::Event_id, 3> event_ids = {Event_id::Hotkey_confirm_pressed, Event_id::Hotkey_cancel_pressed, Event_id::Hotkey_skip_pressed};
        for (std::size_t i = 0; i < 3; i++) {
            connections[i] = QObject::connect(parent, key_signals[i], [this, event_id = event_ids[i]] {
                {
                    std::unique_lock<std::mutex> lock{await_mutex};
                    await_condition = event_id;
                }
                await_condition_variable.notify_one();
            });
        }
        return connections;
    });
    //    MainWindow::mw->execute_in_gui_thread(this, [this, &timer, timeout_ms, &callback_timer] {
    auto disconnector = Utility::RAII_do([connections] {
        for (auto &connection : connections) {
            QObject::disconnect(connection);
        }
    });
    //wait for hotkey or interrupt
    await_condition_variable.wait(lock, [this] {
        return await_condition == Event_id::interrupted || await_condition == Event_id::Hotkey_confirm_pressed ||
               await_condition == Event_id::Hotkey_cancel_pressed || await_condition == Event_id::Hotkey_skip_pressed;
    });
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    //reset condition and return result
    auto result = await_condition;
    await_condition = Event_id::invalid;
    return result;
}

void ScriptEngine::post_ui_event() {
    {
        std::unique_lock<std::mutex> lock{await_mutex};
        await_condition = Event_id::UI_activated;
    }
    await_condition_variable.notify_one();
}

void ScriptEngine::post_hotkey_event(Event_id::Event_id event) {
    {
        std::unique_lock<std::mutex> lock{await_mutex};
        await_condition = event;
    }
    await_condition_variable.notify_one();
}

void ScriptEngine::post_interrupt(QString message) {
    script_interrupted();
    console.error() << "Script interrupted" << (message.isEmpty() ? "" : " because of error: " + message);
    {
        std::unique_lock<std::mutex> lock{await_mutex};
        await_condition = Event_id::interrupted;
    }
    await_condition_variable.notify_one();
}

std::vector<std::string> ScriptEngine::get_default_globals() {
    std::vector<std::string> globals;
    Console console{nullptr};
    ScriptEngine se{nullptr, console, nullptr, {}};
    script_setup(*se.lua, "", se, se.parent, se.console.get_plaintext_edit());
    for (auto &[key, value] : se.lua->globals()) {
        (void)value;
        if (key.is<std::string>()) {
            globals.emplace_back(key.as<std::string>());
        }
    }
    return globals;
}

static std::string to_string(const void *p) {
    std::ostringstream ss;
    ss << p;
    return std::move(ss).str();
}

static std::string to_string(const RPCDevice &device) {
    return "RPCDevice(" + device.protocol->get_device_summary().replace('\n', ", ").toStdString() + ')';
}

std::string ScriptEngine::to_string(double d) {
    if (std::fmod(d, 1.) == 0) {
        return std::to_string(static_cast<long long int>(d));
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
            return "nil";
        case sol::type::none:
            return "nil";
        case sol::type::string:
            return "\"" + o.as<std::string>() + "\"";
        case sol::type::table:
            return to_string(o.as<sol::table>());
        case sol::type::userdata:
            if (o.is<Color>()) {
                return "Ui.Color(0x" + QString::number(o.as<Color>().rgb & 0xFFFFFFu, 16).toStdString() + ")";
            }
            if (o.is<RPCDevice>()) {
                const auto &device = o.as<RPCDevice &>();
                return ::to_string(device);
            }
            if (o.is<DataLogger>()) {
                //const auto &dl = o.as<DataLogger&>();
                return "DataLogger@" + ::to_string(o.pointer());
            }
            if (o.is<SCPIDevice>()) {
                const auto &device = o.as<SCPIDevice &>();
                return "SCPIDevice (Port: " + device.device->getName().toStdString() + ", name: " + device.get_name() +
                       ", Serial Number: " + device.get_serial_number() + ", Manufacturer: " + device.get_manufacturer() + ")";
            }
            if (o.is<SG04CountDevice>()) {
                const auto &device = o.as<SG04CountDevice &>();
                return "SG04Device (Port: " + device.device->getName().toStdString() + ")";
            }
            if (o.is<ManualDevice>()) {
                const auto &device = o.as<ManualDevice &>();
                return "ManualDevice (" + device.protocol->get_summary() + ")";
            }
            if (o.is<Data_engine_handle>()) {
                const auto &engine = o.as<Data_engine_handle &>();
                return "Data engine (Path: " + engine.data_engine->source_path.toStdString() + ")";
            }
            if (o.is<Sol_table>()) {
                throw std::runtime_error(QObject::tr("Sol_table instead of sol::table error in CrystaTestFramework. Its not your fault. Please report at "
                                                     "\"http://candy/redmine/projects/fsngst/issues/new\".")
                                             .toStdString());
            }
            return "unknown custom datatype@" + ::to_string(o.pointer());
        default:
            return "unknown type " + std::to_string(static_cast<int>(o.get_type()));
    }
}

std::string ScriptEngine::to_string(const sol::stack_proxy &object) {
    return to_string(sol::object{object});
}

std::string ScriptEngine::to_string(const sol::table &table) {
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
    } else {
        retval.push_back('}');
    }
    return retval;
}

QString ScriptEngine::get_absolute_filename(QString file_to_open) {
    return get_absolute_file_path(path_m, file_to_open);
}

bool ScriptEngine::adopt_device(const MatchedDevice &device) {
    return runner->adopt_device(device);
}

std::string ScriptEngine::get_script_import_path(const std::string &name) {
    auto script = QString::fromStdString(name);
    if (not script.endsWith(".lua")) {
        script += ".lua";
    }

    QDir current_script_dir{path_m};
    current_script_dir.cdUp();
    for (auto &dir : {current_script_dir, QDir{QSettings{}.value(Globals::test_script_path_settings_key).toString()}, QDir{}}) {
        const auto filepath = dir.filePath(script);
        if (QFile::exists(filepath)) {
            return filepath.toStdString();
        }
    }
    return name;
}

void ScriptEngine::load_script(const std::string &path) {
    path_m = QString::fromStdString(path);

    try {
        script_setup(*lua, path, *this, parent, console.get_plaintext_edit());
        lua->script_file(path);
    } catch (const sol::error &error) {
        qDebug() << "caught sol::error@load_script";
        set_error_line(error);
        throw;
    }
}

void ScriptEngine::set_error_line(const sol::error &error) {
    const std::string &string = error.what();
    std::regex r(R"(\.lua:([0-9]*): )");
    std::smatch match;
    if (std::regex_search(string, match, r)) {
        Utility::convert(match[1].str(), error_line);
    }
}

void ScriptEngine::reset_lua_state() {
    if (lua_devices) {
        lua_devices = std::nullopt;
    }
    lua = std::make_unique<sol::state>();
    lua_devices = lua->create_table_with();

    //register RPC device type
    auto type_reg = lua->new_usertype<RPCDevice>("RPCDevice");
    type_reg.set(sol::meta_function::index, [this](RPCDevice &device, std::string name) -> sol::object {
        abort_check();
        qDebug() << "Checking custom index" << name.c_str();
        if (device.has_function(name)) {
            return sol::object(*lua, sol::in_place, [function_name = std::move(name)](RPCDevice &device, const sol::variadic_args &va) {
                abort_check();
                return device.call_rpc_function(function_name, va, true);
            });
        }
        if (device.enums[name].valid()) {
            return device.enums[name];
        }
        throw std::runtime_error{"Element \"" + name + "\" does not exist in " + ::to_string(device)};
    });
    type_reg.set("try", [this](RPCDevice &device, std::string function_name, const sol::variadic_args &va) {
        abort_check();
        auto result = create_table();
        try {
            result["result"] = device.call_rpc_function(function_name, va, false);
            result["timeout"] = false;
            return result;
        } catch (const RPCTimeoutException &) {
            result["timeout"] = true;
            return result;
        }
    });
    type_reg.set("get_protocol_name", [](RPCDevice &device) {
        abort_check();
        return device.get_protocol_name();
    });
    type_reg.set("is_protocol_device_available", [](RPCDevice &device) {
        abort_check();
        return device.is_protocol_device_available();
    });
    assert(lua_devices);
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
    launch_editor(path_m, error_line);
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
        set_error_line(error);
        throw;
    }
    return retval;
}

static int get_quantity_num(sol::object &obj) {
    int result = 0;
    if (obj.get_type() == sol::type::string) {
        std::string str = obj.as<std::string>();
        QString qstr = QString::fromStdString(str);
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

std::vector<DeviceRequirements> ScriptEngine::get_device_requirement_list(const sol::table &device_requirements) {
    std::vector<DeviceRequirements> result{};
    try {
        const sol::table &protocol_entries = device_requirements;
        if (protocol_entries.valid() == false) {
            return result;
        }
        for (auto &protocol_entry : protocol_entries) {
            DeviceRequirements item{};
            sol::table protocol_entry_table = protocol_entry.second.as<sol::table>();
            bool device_name_found = false;
            bool protocol_does_not_provide_name = false;
            for (auto &protocol_entry_field : protocol_entry_table) {
                const std::string key = protocol_entry_field.first.as<std::string>();
                if (key == "protocol") {
                    item.protocol_name = QString::fromStdString(protocol_entry_field.second.as<std::string>());
                    if (item.protocol_name == "SG04Count") {
                        protocol_does_not_provide_name = true;
                    }
                } else if (key == "device_names") {
                    device_name_found = true;
                    if (protocol_entry_field.second.get_type() == sol::type::table) {
                        sol::table device_names = protocol_entry_field.second.as<sol::table>();
                        for (auto &device_name : device_names) {
                            std::string str = device_name.second.as<std::string>();
                            if (str == "") {
                                str = "*";
                            }
                            item.device_names.append(QString::fromStdString(str));
                        }
                    }
                    if (protocol_entry_field.second.get_type() == sol::type::string) {
                        std::string str = protocol_entry_field.second.as<std::string>();
                        if (str == "") {
                            str = "*";
                        }
                        item.device_names.append(QString::fromStdString(str));
                    }
                } else if (key == "quantity") {
                    item.quantity_min = get_quantity_num(protocol_entry_field.second);
                    item.quantity_max = item.quantity_min;
                } else if (key == "quantity_min") {
                    item.quantity_min = get_quantity_num(protocol_entry_field.second);
                } else if (key == "quantity_max") {
                    item.quantity_max = get_quantity_num(protocol_entry_field.second);
                } else if (key == "alias") {
                    item.alias = QString::fromStdString(protocol_entry_field.second.as<std::string>());
                } else if (key == "acceptable") {
                    if (not protocol_entry_field.second.is<sol::function>()) {
                        throw std::runtime_error("The \"acceptable\" field in device requirements must be a function or left out.");
                    }
                    item.has_acceptance_function = true;
                } else {
                    console.warning() << QObject::tr("Ignored device requirement \"%1\" because it is not a known requirement.").arg(key.c_str());
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
    }

    catch (const sol::error &error) {
        set_error_line(error);
        throw;
    }
    return result;
}

sol::table ScriptEngine::get_device_requirements_table() {
    auto table = lua->get<sol::optional<sol::table>>("device_requirements");
    if (table) {
        return table.value();
    }
    return create_table();
}

std::string ScriptEngine::device_list_string() {
    return Utility::promised_thread_call(this, [this] {
        if (currently_in_gui_thread()) {
            return final_device_list_string;
        }
        return lua_devices.has_value() ? to_string(*lua_devices) : "{}";
    });
}

std::vector<DeviceRequirements> ScriptEngine::get_device_requirement_list() {
    try {
        auto requirements_table = lua->get<sol::optional<sol::table>>("device_requirements");
        if (requirements_table) {
            return get_device_requirement_list(requirements_table.value());
        }
        return {};
    } catch (const sol::error &error) {
        set_error_line(error);
        throw;
    }
}

sol::table ScriptEngine::get_devices(const std::vector<MatchedDevice> &devices) {
    QMultiMap<QString, MatchedDevice> aliased_devices;
    for (auto &device_protocol : devices) {
        aliased_devices.insertMulti(device_protocol.proposed_alias, device_protocol);
    }
    std::vector<sol::object> no_alias_device_list;
    std::map<QString, std::vector<sol::object>> aliased_devices_result;

    //creating devices..
    auto aliases = aliased_devices.keys();
    for (auto alias : aliases) {
        auto values = aliased_devices.values(alias);
        std::vector<sol::object> device_list;
        for (const auto &device_protocol : values) {
            if (auto rpcp = dynamic_cast<RPCProtocol *>(device_protocol.protocol)) {
                auto enums = create_table();
                for (const auto &function : rpcp->get_description().get_functions()) {
                    add_enum_types(function, *lua, enums);
                }
                sol::object rpc_device_sol(*lua, sol::in_place, RPCDevice{&*lua, rpcp, device_protocol.device, this, std::move(enums)});
                while (device_protocol.device->waitReceived(CommunicationDevice::Duration{0}, 1)) {
                    //ignore leftover data in the receive buffer
                }
                rpcp->clear();
                device_list.push_back(rpc_device_sol);

            } else if (auto scpip = dynamic_cast<SCPIProtocol *>(device_protocol.protocol)) {
                sol::object scpip_device_sol = sol::make_object(*lua, SCPIDevice{&*lua, scpip, device_protocol.device, this});
                device_list.push_back(scpip_device_sol);
                scpip->clear();
            } else if (auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(device_protocol.protocol)) {
                sol::object sg04_device_sol = sol::make_object(*lua, SG04CountDevice{&*lua, sg04_count_protocol, device_protocol.device, this});
                device_list.push_back(sg04_device_sol);
            } else if (auto manual_protocol = dynamic_cast<ManualProtocol *>(device_protocol.protocol)) {
                sol::object manual_device_sol = sol::make_object(*lua, ManualDevice{&*lua, manual_protocol, device_protocol.device, this});
                device_list.push_back(manual_device_sol);
            } else {
                //TODO: other protocols
                throw std::runtime_error("invalid protocol: " + device_protocol.protocol->type.toStdString());
            }
        }
        if (alias == "") {
            no_alias_device_list.insert(no_alias_device_list.end(), device_list.begin(), device_list.end());
        } else {
            aliased_devices_result[alias] = device_list;
        }
    }

    //ordering/grouping devices..
    if (!lua_devices) {
        lua_devices = lua->create_table_with();
    }
    for (auto sol_device : no_alias_device_list) {
        lua_devices->add(sol_device);
    }
    for (const auto &alias : aliased_devices_result) {
        auto values = alias.second;
        if (values.size() == 1) {
            (*lua_devices)[alias.first.toStdString()] = values[0];
        } else {
            auto devices_with_same_alias = lua->create_table_with();
            for (const auto &value : values) {
                devices_with_same_alias.add(value);
            }
            (*lua_devices)[alias.first.toStdString()] = devices_with_same_alias;
        }
    }

    return *lua_devices;
}

void ScriptEngine::run(std::vector<MatchedDevice> &devices) {
    assert(lua_devices);
    qDebug() << "ScriptEngine::run";
    assert(not currently_in_gui_thread());
    matched_devices = &devices;
    try {
        {
            sol::protected_function run = (*lua)["run"];
            if (not run.valid()) {
                throw std::runtime_error{"Script does not have a \"run\" function."};
            }
            auto result = run(get_devices(devices));
            if (not result.valid()) {
                sol::error error = result;
                throw error;
            }
        }
        script_finished();
        reset_lua_state();
    } catch (const sol::error &e) {
        qDebug() << "caught sol::error@run";
        final_device_list_string = to_string(*lua_devices);
        set_error_line(e);
        reset_lua_state();
        throw;
    } catch (...) {
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
