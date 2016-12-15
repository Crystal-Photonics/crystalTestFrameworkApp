#include "scriptengine.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
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
	sol::object operator()(const std::string &name, sol::variadic_args va) {
		//return sol::make_object(lua, 42);
		return sol::nil;
	}
	lua_State *lua = nullptr;
};

void ScriptEngine::run(std::list<DeviceProtocol> device_protocols) {
	try {
		std::vector<std::function<sol::object(const std::string &name, sol::variadic_args va)>> device_callback;
		auto device_list = lua.create_table_with();
		for (auto &device_protocol : device_protocols) {
			if (auto rpcp = dynamic_cast<RPCProtocol *>(&device_protocol.protocol)) {
				device_callback.push_back(RPCDevice{lua.lua_state()});
				auto type = lua.create_simple_usertype<RPCDevice>();
				const auto &type_name = "RPCDevice_" + rpcp->get_description().get_hash();
				lua.set_usertype(type_name, type);
				for (auto &function : rpcp->get_description().get_functions()) {
					const auto &function_name = function.get_function_name();
					lua[type_name][function_name] = [ function_name, &device = device_callback.back() ](RPCDevice &self, sol::variadic_args va) {
						device(function_name, va);
					};
				}
				//TODO: create an object of type type and add it to device_list
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
