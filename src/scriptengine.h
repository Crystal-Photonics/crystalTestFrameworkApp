#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "CommunicationDevices/communicationdevice.h"
#include "Protocols/protocol.h"
#include "export.h"
#include "luaui.h"
#include "sol.hpp"

#include <QFile>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

struct DeviceProtocol {
	CommunicationDevice &device;
	Protocol &protocol;
};


class ScriptEngine {
	public:
	enum class State { idle, running, aborting, done };
	friend class TestRunner;
	friend class TestDescriptionLoader;
	friend class DeviceWorker;
	friend struct RPCDevice;

	ScriptEngine(LuaUI &&lua_ui);
	void load_script(const QString &path);
	void launch_editor() const;

	private: //note: most of these things are private so that the GUI thread does not access anything important. Do not make things public.
	LuaUI &get_ui();
	sol::table create_table();
	State state = State::idle;
	QStringList get_string_list(const QString &name);
	void run(std::list<DeviceProtocol> device_protocols, std::function<void(std::list<DeviceProtocol> &)> debug_callback = [](std::list<DeviceProtocol> &) {});
	template <class ReturnType, class... Arguments>
	ReturnType call(const char *function_name, Arguments &&... args);

	void set_error(const sol::error &error);
	sol::state lua;
	QString path;
	int error_line = 0;
	std::unique_ptr<LuaUI> lua_ui;
	std::unique_ptr<std::mutex> state_is_idle = std::make_unique<std::mutex>(); //is free when state==idle and locked otherwise
};

template <class ReturnType, class... Arguments>
ReturnType ScriptEngine::call(const char *function_name, Arguments &&... args) {
	sol::protected_function f = lua[function_name];
	auto call_res = f(std::forward<Arguments>(args)...);
	if (call_res.valid()) {
		return call_res;
	}
	sol::error error = call_res;
	set_error(error);
	throw error;
}

#endif // SCRIPTENGINE_H
