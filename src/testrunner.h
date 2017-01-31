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

class TestRunner : QObject {
	Q_OBJECT
	public:
	TestRunner();
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

#if 0
struct Test {
	Test(QTreeWidget *test_list, const QString &file_path);
	~Test();
	Test(const Test &) = delete;
	Test(Test &&other);
	Test &operator=(const Test &) = delete;
	Test &operator=(Test &&other);
	void swap(Test &other);
	void reset_ui();

	QTreeWidget *parent = nullptr;
	QTreeWidgetItem *ui_item = nullptr;
	QPlainTextEdit *console = nullptr;
	QSplitter *test_console_widget = nullptr;
	ScriptEngine script;
	std::vector<QString> protocols;
	QString name;
	QString file_path;
	bool operator==(QTreeWidgetItem *item);
	std::unique_ptr<Worker> worker;
	std::unique_ptr<QThread> worker_thread;
};
#endif

#if 0
void await_idle(ScriptEngine &script);
void set_gui_parent(ScriptEngine &script, QSplitter *parent);

public slots:
ScriptEngine::State get_state(ScriptEngine &script);
void abort_script(ScriptEngine &script);
#endif

#endif // TESTRUNNER_H
