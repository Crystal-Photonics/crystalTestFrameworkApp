#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "data_engine/data_engine.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "sol.hpp"

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

class TestRunner : QObject {
    Q_OBJECT
    public:
	enum class State { running, finished, error };
    using Lua_ui_container = QWidget;
    TestRunner(const TestDescriptionLoader &description);
    ~TestRunner();

    void interrupt();
    void join();
    void pause_timers();
    void resume_timers();
    sol::table create_table();
    template <class ReturnType, class... Arguments>
    ReturnType call(const char *function_name, Arguments &&... args);
    UI_container *get_lua_ui_container() const;
    void run_script(std::vector<MatchedDevice> devices, DeviceWorker &device_worker);
    bool is_running() const;
    const QString &get_name() const;
    void launch_editor() const;

    QPlainTextEdit *console{nullptr};

    QObject *obj();

    private:
    QThread thread{};
    UI_container *lua_ui_container{nullptr};
    std::unique_ptr<Data_engine> data_engine{std::make_unique<Data_engine>()};
    ScriptEngine script;
    QString name{};
};

template <class ReturnType, class... Arguments>
ReturnType TestRunner::call(const char *function_name, Arguments &&... args) {
    ReturnType p = Utility::promised_thread_call(this,
                                                 [this, function_name, &p, &args...] { //
                                                     auto result = script.call<ReturnType>(function_name, std::forward<Arguments>(args)...);
                                                     return result;
                                                 } //
                                                 );
    return p;
}

#endif // TESTRUNNER_H
