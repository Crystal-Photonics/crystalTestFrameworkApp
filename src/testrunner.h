#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "scriptengine.h"
#include "sol.hpp"
#include "qt_util.h"

#include <QObject>
#include <QThread>
#include <future>

class QPlainTextEdit;
class QSplitter;
struct LuaUI;
struct ComportDescription;
class TestDescriptionLoader;

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
	void run_script(ComportDescription &device);
	bool is_running();
	QString get_name();

	private:
	QThread thread;
	QSplitter *lua_ui_container = nullptr;
	QPlainTextEdit *console = nullptr;
	ScriptEngine script;
};

template <class ReturnType, class... Arguments>
ReturnType TestRunner::call(const char *function_name, Arguments &&... args) {
	std::promise<ReturnType> p;
	auto f = p.get_future();
	Utility::thread_call(this, [this, function_name, &p, &args...]{
		p.set_value(script.call<ReturnType>(function_name, std::forward<Arguments>(args)...));
	});
	return f.get();
}

#endif // TESTRUNNER_H
