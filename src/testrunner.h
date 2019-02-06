#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "forward_decls.h"
#include "qt_util.h"

#include <QObject>
#include <QPlainTextEdit>
#include <QThread>
#include <future>

class CommunicationDevice;
class DeviceWorker;
class QPlainTextEdit;
class QSplitter;
class TestDescriptionLoader;
class UI_container;
struct LuaUI;
struct Protocol;
struct Sol_table;
struct MatchedDevice;
class Data_engine;

class TestRunner : QObject {
    Q_OBJECT
    public:
    using Lua_ui_container = QWidget;
    TestRunner(const TestDescriptionLoader &description);
    ~TestRunner();

    void interrupt();
    void message_queue_join();
    void blocking_join();
	Sol_table create_table();
	template <class Callback>
	auto call(Callback &&cb) { /* this is a complicated way of saying script.call(lua_function);, but we do it in
								  order to not have to include sol.hpp here to improve compilation time */
		return Utility::promised_thread_call(this,
											 [ this, callback = std::forward<Callback>(cb) ] { return std::forward<decltype(callback)>(callback)(script); });
	}

    UI_container *get_lua_ui_container() const;
    void run_script(std::vector<MatchedDevice> devices, DeviceWorker &device_worker);
    bool is_running() const;
    const QString &get_name() const;
    void launch_editor() const;

    QPlainTextEdit *console{nullptr};

    QObject *obj();

    private:
    Utility::Qt_thread thread{};
    UI_container *lua_ui_container{nullptr};
	std::unique_ptr<Data_engine> data_engine;
	std::unique_ptr<ScriptEngine> script_pointer;
	ScriptEngine &script;
    QString name{};
};

#endif // TESTRUNNER_H
