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
#include <memory>
#include <mutex>
#include <vector>

class ScriptEngine {
	public:
	friend class TestRunner;
	friend class TestDescriptionLoader;
	friend class DeviceWorker;

	ScriptEngine(LuaUI &&lua_ui);
	void load_script(const QString &path);
	static void launch_editor(QString path, int error_line = 1);
	void launch_editor() const;

	private: //note: most of these things are private so that the GUI thread does not access anything important. Do not make things public.
	LuaUI &get_ui();
	sol::table create_table();
	QStringList get_string_list(const QString &name);
	void run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices);
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
