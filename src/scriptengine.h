#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "qt_util.h"

#include <QEventLoop>
#include <QList>
#include <QObject>
#include <QString>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <sol.hpp>
#include <vector>

class QStringList;
class QSplitter;
class QPlainTextEdit;
struct Protocol;
class Data_engine;
class UI_container;
class TestRunner;
struct MatchedDevice;
struct Console;

//wrappers to get around not being able to forward-declare sol::table
struct Sol_table {
    sol::table table;
    Sol_table(sol::table table)
        : table{std::move(table)} {}
    operator sol::table &() {
        return table;
    }
    operator const sol::table &() const {
        return table;
    }
};

QString get_absolute_file_path(const QString &script_path, const QString &file_to_open);
std::string get_absolute_file_path(const QString &script_path, const std::string &file_to_open);

class DeviceRequirements {
    public:
    QString protocol_name;
    QStringList device_names;
    int quantity_min = 0;
    int quantity_max = INT_MAX;
    QString get_description() const;
    QString alias;
    bool has_acceptance_function = false;
};

namespace Event_id {
    enum Event_id { Hotkey_confirm_pressed, Hotkey_skip_pressed, Hotkey_cancel_pressed, Timer_expired, UI_activated, interrupted = -1, invalid = -2 };
}

class ScriptEngine : public QObject {
    Q_OBJECT

    signals:
    void script_interrupted();
    void script_finished();

    public:
    friend class TestRunner;
    friend class TestDescriptionLoader;
    friend class DeviceWorker;

    ScriptEngine(UI_container *parent, Console &console, TestRunner *runner, QString test_name);
    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine(ScriptEngine &&) = delete;
    ~ScriptEngine();

    Event_id::Event_id await_timeout(std::chrono::milliseconds duration, std::chrono::milliseconds start = {});
    Event_id::Event_id await_ui_event();
    Event_id::Event_id await_hotkey_event();

    void post_hotkey_event(Event_id::Event_id event);
    void post_ui_event();
    void post_interrupt(QString message = {});

    static std::vector<std::string> get_default_globals();

    void load_script(const std::string &path);
    static void launch_editor(QString path, int error_line = 1);
    void launch_editor() const;
    static std::string to_string(double d);
    static std::string to_string(const sol::object &o);
    static std::string to_string(const sol::stack_proxy &object);
    static std::string to_string(const sol::table &table);
    QString get_absolute_filename(QString file_to_open);
    bool adopt_device(const MatchedDevice &device);
    std::string get_script_import_path(const std::string &name);

    sol::table create_table();
    template <class Return_type, class... Args>
    Return_type call_lua_function(const char *lua_function, Args &&... args) {
        return Utility::promised_thread_call(this, [this, lua_function, &args...] { return call<Return_type>(lua_function, std::forward<Args>(args)...); });
    }
    TestRunner *runner;
    QString test_name;
    std::vector<DeviceRequirements> get_device_requirement_list(const sol::table &device_requirements);
    sol::table get_device_requirements_table(); //The returned table must be destroyed before the script. TODO: Fix or diagnose better
    std::string device_list_string();

    private: //note: most of these things are private so that the GUI thread does not access anything belonging to the script thread. Do not make things public.
    QStringList get_string_list(const QString &name);
    std::vector<DeviceRequirements> get_device_requirement_list();
    sol::table get_devices(const std::vector<MatchedDevice> &devices);
    void run(std::vector<MatchedDevice> &devices);
    template <class ReturnType, class... Arguments>
    ReturnType call(const char *function_name, Arguments &&... args);
    void set_error_line(const sol::error &error);
    void reset_lua_state();

    std::optional<sol::table> lua_devices;
    std::unique_ptr<sol::state> lua{};
    QString path_m{};
    int error_line{0};
    UI_container *parent{nullptr};
    Console &console;
    QObject *owner;
    std::vector<MatchedDevice> *matched_devices;
    std::string final_device_list_string;

    std::mutex await_mutex;
    std::condition_variable await_condition_variable;
    Event_id::Event_id await_condition = Event_id::invalid;

    friend void bind_dataengineinput(sol::state &lua, sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent, const std::string &path);
    friend void bind_lua_functions(sol::state &lua, sol::table &ui_table, const std::string &path, ScriptEngine &script_engine, QPlainTextEdit *console);
};

template <class ReturnType, class... Arguments>
ReturnType ScriptEngine::call(const char *function_name, Arguments &&... args) {
    sol::protected_function f = (*lua)[function_name];
    auto call_res = f(std::forward<Arguments>(args)...);
    if (call_res.valid()) {
        return call_res;
    }
    sol::error error = call_res;
    set_error_line(error);
    throw error;
}

template <class ReturnType = void, class... Arguments>
ReturnType sol_call(sol::state &lua, const char *function_name, Arguments &&... args) {
    sol::protected_function f = lua[function_name];
    auto call_res = f(std::forward<Arguments>(args)...);
    if (call_res.valid()) {
        return static_cast<ReturnType>(call_res);
    }
    sol::error error = call_res;
    throw error;
}

#endif // SCRIPTENGINE_H
