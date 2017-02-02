#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "qt_util.h"
#include "scriptengine.h"
#include "sol.hpp"

#include <QObject>
#include <QThread>
#include <future>

class CommunicationDevice;
class DeviceWorker;
class QPlainTextEdit;
class QSplitter;
class TestDescriptionLoader;
struct LuaUI;
struct Protocol;

class TestRunner : QObject {
	Q_OBJECT
	public:
	TestRunner(const TestDescriptionLoader &description);
	~TestRunner();
	void interrupt();
	void join();
	sol::table create_table();
	template <class ReturnType, class... Arguments>
	ReturnType call(const char *function_name, Arguments &&... args);
	QSplitter *get_lua_ui_container() const;
	void run_script(std::vector<std::pair<CommunicationDevice *, Protocol *>> devices, const DeviceWorker &device_worker);
	bool is_running();
	const QString &get_name() const;
	void launch_editor() const;

	private:
	QThread thread;
	QSplitter *lua_ui_container = nullptr;
	QPlainTextEdit *console = nullptr;
	ScriptEngine script;
	QString name;
};

template <class ReturnType, class... Arguments>
ReturnType TestRunner::call(const char *function_name, Arguments &&... args) {
	std::promise<ReturnType> p;
	auto f = p.get_future();
	Utility::thread_call(this, [this, function_name, &p, &args...] { p.set_value(script.call<ReturnType>(function_name, std::forward<Arguments>(args)...)); });
	return f.get();
}

#endif // TESTRUNNER_H
