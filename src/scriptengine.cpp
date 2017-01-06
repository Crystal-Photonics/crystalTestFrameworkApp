#include "scriptengine.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_encoded_function_call.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <functional>
#include <regex>
#include <string>
#include <vector>

ScriptEngine::ScriptEngine(LuaUI lua_ui)
	: lua_ui(std::move(lua_ui)) {}

void ScriptEngine::load_script(const QString &path) {
	//NOTE: When using lambdas do not capture this or by reference, because it breaks when the ScriptEngine is moved
	this->path = path;
	try {
		lua.open_libraries(sol::lib::base); //load the standard lib if necessary
		lua.set_function("show_warning", [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
			QMessageBox::warning(nullptr, QString::fromStdString(title.value_or("nil")) + " from " + path, QString::fromStdString(message.value_or("nil")));
		});
		lua.script_file(path.toStdString());

		//bind UI
		auto ui_table = lua.create_named_table("ui");
		ui_table.new_usertype<Plot>("plot",                                                                                  //
									sol::meta_function::construct, sol::no_constructor,                                      //
									sol::meta_function::construct, [lua_ui = this->lua_ui] { return lua_ui.create_plot(); }, //
									"add", &Plot::add,                                                                       //
									"clear", &Plot::clear);
		ui_table["create_table"] = [lua_ui = this->lua_ui] {
			lua_ui.create_plot();
		};

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
			return sol::make_object(lua.lua_state(), "TODO: Parse return value of type array");
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
		//Console::note() << QString("RPC Device got called with function \"%1\" and %2 arguments").arg(name.c_str()).arg(va.leftover_count());
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
			//TODO: call function and return return value
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
					return sol::make_object(lua->lua_state(), "TODO: Parse multiple return values");
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
};

void ScriptEngine::run(std::list<DeviceProtocol> device_protocols, std::function<void(std::list<DeviceProtocol> &)> debug_callback) {
	try {
		auto device_list = lua.create_table_with();
		for (auto &device_protocol : device_protocols) {
			if (auto rpcp = dynamic_cast<RPCProtocol *>(&device_protocol.protocol)) {
				device_list.add(RPCDevice{&lua, rpcp, &device_protocol.device});
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
		lua["run"](device_list);
	} catch (const sol::error &e) {
		debug_callback(device_protocols);
		set_error(e);
		throw;
	}
}
