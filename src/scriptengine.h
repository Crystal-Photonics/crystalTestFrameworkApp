#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "export.h"
#include "sol.hpp"
#include "CommunicationDevices/communicationdevice.h"
#include "Protocols/protocol.h"

#include <QFile>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <list>

struct DeviceProtocol{
	CommunicationDevice &device;
	Protocol &protocol;
};

class ScriptEngine {
	public:
	void load_script(const QString &path);
	QStringList get_string_list(const QString &name);
	void launch_editor() const;
	sol::table create_table();
	void run(std::list<DeviceProtocol> device_protocols);
	template <class ReturnType, class... Arguments>
	ReturnType call(const char *function_name, Arguments &&... args);

	private:
	void set_error(const sol::error &error);
	sol::state lua;
	QString path;
	int error_line = 0;
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
