#include "scriptengine.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
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

void ScriptEngine::load_script(const QString &path) {
	this->path = path;
	try {
		lua.open_libraries(sol::lib::base); //load the standard lib if necessary
		lua.set_function("show_warning", [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
			QMessageBox::warning(nullptr, QString::fromStdString(title.value_or("nil")) + " from " + path, QString::fromStdString(message.value_or("nil")));
		});
		lua.script_file(path.toStdString());
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

struct RPCDevice {
	sol::object call_rpc_function(const std::string &name, sol::variadic_args va) {
		Console::note() << QString("RPC Device got called with function \"%1\" and %2 arguments").arg(name.c_str()).arg(va.leftover_count());
		auto function = protocol->encode_function(name);
		//TODO: put arguments from va into function
		if (function.are_all_values_set()) {
			//TODO: call function
		} else {
			//not all values set, error
			throw sol::error("Failed calling function, missing parameters");
		}
		//return sol::make_object(lua, 42);
		return sol::nil;
	}
	sol::state *lua = nullptr;
	RPCProtocol *protocol = nullptr;
};

void ScriptEngine::run(std::list<DeviceProtocol> device_protocols) {
	try {
		auto device_list = lua.create_table_with();
		for (auto &device_protocol : device_protocols) {
			if (auto rpcp = dynamic_cast<RPCProtocol *>(&device_protocol.protocol)) {
				device_list.add(RPCDevice{&lua, rpcp});
				auto type_reg = lua.create_simple_usertype<RPCDevice>();
				for (auto &function : rpcp->get_description().get_functions()) {
					const auto &function_name = function.get_function_name();
					type_reg.set(function_name, [function_name](RPCDevice &device, sol::variadic_args va) { device.call_rpc_function(function_name, va); });
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
		set_error(e);
		throw;
	}
}
