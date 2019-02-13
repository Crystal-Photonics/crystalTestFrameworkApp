#include "scriptengine.h"
#include "LuaUI/color.h"
#include "Protocols/rpcprotocol.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "chargecounter.h"
#include "communication_devices.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h"
#include "datalogger.h"
#include "lua_functions.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_function.h"
#include "scriptsetup.h"
#include "testrunner.h"
#include "ui_container.h"
#include "util.h"

#include "data_engine/exceptionalapproval.h"
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
#include <QTimer>
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
    bool is_protocol_device_available() {
#if 1
        auto result = protocol->call_get_hash_function(0);
        if (result.decoded_function_call_reply) {
            return true;
        }
#endif
        return false;
    }

    sol::object call_rpc_function(const std::string &name, const sol::variadic_args &va) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            throw sol::error("Abort Requested");
        }

		Console_handle::note() << QString("\"%1\" called").arg(name.c_str());
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
					Console_handle::error() << e.what();
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

ScriptEngine::ScriptEngine(QObject *owner, UI_container *parent, Console &console, Data_engine *data_engine)
    : lua{std::make_unique<sol::state>()}
    , parent{parent}
    , console(console)
    , data_engine(data_engine)
    , owner{owner} {}

ScriptEngine::~ScriptEngine() { //
    // qDebug() << "script destruktor " << (QThread::currentThread() == MainWindow::gui_thread ? "(GUI Thread)" : "(Script Thread)") <<
    // QThread::currentThread();
}

Event_id::Event_id ScriptEngine::await_timeout(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock{await_mutex};
    if (await_condition == Event_id::interrupted) {
        throw std::runtime_error("Interrupted");
    }
    await_condition_variable.wait_for(lock, timeout, [this] { return await_condition == Event_id::interrupted; });
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
        std::array<void (MainWindow::*)(), 3> key_signals = {&MainWindow::confirm_key_sequence_pressed, &MainWindow::cancel_key_sequence_key_pressed,
                                                             &MainWindow::skip_key_sequence_pressed};
        std::array<Event_id::Event_id, 3> event_ids = {Event_id::Hotkey_confirm_pressed, Event_id::Hotkey_cancel_pressed, Event_id::Hotkey_skip_pressed};
        for (std::size_t i = 0; i < 3; i++) {
			connections[i] = QObject::connect(MainWindow::mw, key_signals[i], [ this, event_id = event_ids[i] ] {
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
	Utility::thread_call(MainWindow::mw, [this, message] { console.error() << "Script interrupted" << (message.isEmpty() ? "" : "because of " + message); });
    {
        std::unique_lock<std::mutex> lock{await_mutex};
        await_condition = Event_id::interrupted;
	}
    await_condition_variable.notify_one();
}

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

QString ScriptEngine::get_absolute_filename(QString file_to_open) {
    return get_absolute_file_path(path_m, file_to_open);
}

void ScriptEngine::load_script(const std::string &path) {
    // qDebug() << "load_script " << QString::fromStdString(path);
    // qDebug() << (QThread::currentThread() == MainWindow::gui_thread ? "(GUI Thread)" : "(Script Thread)") << QThread::currentThread();

    this->path_m = QString::fromStdString(path);

    if (data_engine) {
        data_engine->set_script_path(path_m);
    }

    try {
        script_setup(*lua, path, *this);

        lua->script_file(path);

		//qDebug() << "loaded script"; // << lua->;
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
int get_quantity_num(sol::object &obj) {
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

std::vector<DeviceRequirements> ScriptEngine::get_device_requirement_list(const QString &name) {
    std::vector<DeviceRequirements> result{};
    try {
#if 1
        sol::table protocol_entries = lua->get<sol::table>(name.toStdString());
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
                    item.protocol_name = QString::fromStdString(protocol_entry_field.second.as<std::string>());
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
                } else if (protocol_entry_field.first.as<std::string>() == "quantity") {
                    item.quantity_min = get_quantity_num(protocol_entry_field.second);
                    item.quantity_max = item.quantity_min;
                } else if (protocol_entry_field.first.as<std::string>() == "quantity_min") {
                    item.quantity_min = get_quantity_num(protocol_entry_field.second);
                } else if (protocol_entry_field.first.as<std::string>() == "quantity_max") {
                    item.quantity_max = get_quantity_num(protocol_entry_field.second);
                } else if (protocol_entry_field.first.as<std::string>() == "alias") {
                    item.alias = QString::fromStdString(protocol_entry_field.second.as<std::string>());
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
#endif
    }

    catch (const sol::error &error) {
        set_error_line(error);
        throw;
    }
    return result;
}

void ScriptEngine::run(std::vector<MatchedDevice> &devices) {
    qDebug() << "ScriptEngine::run";
	assert(not currently_in_gui_thread());

    auto reset_lua_state = [this] {
        ExceptionalApprovalDB ea_db{QSettings{}.value(Globals::path_to_excpetional_approval_db_key, "").toString()};
        data_engine->do_exceptional_approvals(ea_db, MainWindow::mw);
        lua = std::make_unique<sol::state>();
        data_engine->save_actual_value_statistic();
        if (data_engine_pdf_template_path.count() and data_engine_auto_dump_path.count()) {
            QFileInfo fi(data_engine_auto_dump_path);
            QString suffix = fi.completeSuffix();
            if (suffix == "") {
                QString path = append_separator_to_path(fi.absoluteFilePath());
                path += "report.pdf";
                fi.setFile(path);
            }
            //fi.baseName("/home/abc/report.pdf") = "report"
            //fi.absolutePath("/home/abc/report.pdf") = "/home/abc/"

            std::string target_filename = propose_unique_filename_by_datetime(fi.absolutePath().toStdString(), fi.baseName().toStdString(), ".pdf");
            target_filename.resize(target_filename.size() - 4);

            data_engine->generate_pdf(data_engine_pdf_template_path.toStdString(), target_filename + ".pdf");
			data_engine->set_log_file(target_filename + "_log.txt");
            if (additional_pdf_path.count()) {
                QFile::copy(QString::fromStdString(target_filename), additional_pdf_path);
			}
        }
        if (data_engine_auto_dump_path.count()) {
            QFileInfo fi(data_engine_auto_dump_path);
            QString suffix = fi.completeSuffix();
            if (suffix == "") {
                QString path = append_separator_to_path(fi.absoluteFilePath());
                path += "dump.json";
                fi.setFile(path);
            }
            std::string json_target_filename = propose_unique_filename_by_datetime(fi.absolutePath().toStdString(), fi.baseName().toStdString(), ".json");
            try {
                data_engine->save_to_json(QString::fromStdString(json_target_filename));
            } catch (const DataEngineError &e) {
                qDebug() << "Failed dumping data to json: " << e.what() << " because of " << static_cast<int>(e.get_error_number());
            }
        }
    };
    try {
        {
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
                        sol::object rpc_device_sol = sol::make_object(*lua, RPCDevice{&*lua, rpcp, device_protocol.device, this});
                        device_list.push_back(rpc_device_sol);

                        auto type_reg = lua->create_simple_usertype<RPCDevice>();
                        for (auto &function : rpcp->get_description().get_functions()) {
                            const auto &function_name = function.get_function_name();
                            type_reg.set(function_name, [function_name](RPCDevice &device, const sol::variadic_args &va) {
                                return device.call_rpc_function(function_name, va);
                            });
                            add_enum_types(function, *lua);
                        }
                        type_reg.set("get_protocol_name", [](RPCDevice &device) { return device.get_protocol_name(); });
                        type_reg.set("is_protocol_device_available", [](RPCDevice &device) { return device.is_protocol_device_available(); });
                        const auto &type_name = "RPCDevice_" + rpcp->get_description().get_hash();
                        lua->set_usertype(type_name, type_reg);
                        while (device_protocol.device->waitReceived(CommunicationDevice::Duration{0}, 1)) {
                            //ignore leftover data in the receive buffer
                        }
                        rpcp->clear();

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
            auto device_list_sol = lua->create_table_with();
            for (auto sol_device : no_alias_device_list) {
                device_list_sol.add(sol_device);
            }
            for (const auto &alias : aliased_devices_result) {
                auto values = alias.second;
                if (values.size() == 1) {
                    device_list_sol[alias.first.toStdString()] = values[0];
                } else {
                    auto devices_with_same_alias = lua->create_table_with();
                    for (const auto &value : values) {
                        devices_with_same_alias.add(value);
                    }
                    device_list_sol[alias.first.toStdString()] = devices_with_same_alias;
                }
            }
            sol::protected_function run = (*lua)["run"];
            auto result = run(device_list_sol);
            if (not result.valid()) {
                sol::error error = result;
                throw error;
            }
        }
        reset_lua_state();
    } catch (const sol::error &e) {
        qDebug() << "caught sol::error@run";
        set_error_line(e);
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
