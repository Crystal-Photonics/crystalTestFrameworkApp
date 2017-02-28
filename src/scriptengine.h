#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "sol.hpp"

#include <QString>
#include <functional>
#include <memory>
#include <vector>
#include <QDebug>

class CommunicationDevice;
class QStringList;
class QSplitter;
class QPlainTextEdit;
struct LuaUI;
struct Protocol;

class ScriptEngine {
	public:
	friend class TestRunner;
	friend class TestDescriptionLoader;
	friend class DeviceWorker;

	ScriptEngine(QSplitter *parent, QPlainTextEdit *console);
	~ScriptEngine();
	void load_script(const QString &path);
	static void launch_editor(QString path, int error_line = 1);
	void launch_editor() const;

	private: //note: most of these things are private so that the GUI thread does not access anything important. Do not make things public.
	sol::table create_table();
	QStringList get_string_list(const QString &name);
	void run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices);
	template <class ReturnType, class... Arguments>
	ReturnType call(const char *function_name, Arguments &&... args);

	void set_error(const sol::error &error);
	std::unique_ptr<sol::state> lua{};
	QString path{};
	int error_line{0};
	QSplitter *parent{nullptr};
	QPlainTextEdit *console{nullptr};
};

template <class ReturnType, class... Arguments>
ReturnType ScriptEngine::call(const char *function_name, Arguments &&... args) {
	sol::protected_function f = (*lua)[function_name];
	auto call_res = f(std::forward<Arguments>(args)...);
	if (call_res.valid()) {
		return call_res;
	}
	sol::error error = call_res;
	set_error(error);
	throw error;
}

#endif // SCRIPTENGINE_H
