#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "sol.hpp"

#include <QDebug>
#include <QEventLoop>
#include <QList>
#include <QString>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class CommunicationDevice;
class QStringList;
class QSplitter;
class QPlainTextEdit;
struct Protocol;
class Data_engine;
class UI_container;
class TestRunner;
struct MatchedDevice;

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
};

namespace Event_id {
    enum Event_id { Hotkey_confirm_pressed, Hotkey_skip_pressed, Hotkey_cancel_pressed, Timer_expired, UI_activated, interrupted = -1, invalid = -2 };
}

class ScriptEngine {
    public:
    friend class TestRunner;
    friend class TestDescriptionLoader;
    friend class DeviceWorker;

    ScriptEngine(TestRunner *owner, UI_container *parent, QPlainTextEdit *console, Data_engine *data_engine);
    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine(ScriptEngine &&) = delete;
    ~ScriptEngine();

    //TimerEvent::TimerEvent timer_event_queue_run(int timeout_ms);
    //UiEvent::UiEvent ui_event_queue_run();
    //HotKeyEvent::HotKeyEvent hotkey_event_queue_run();

    Event_id::Event_id await_timeout(std::chrono::milliseconds timeout);
    Event_id::Event_id await_ui_event();
    Event_id::Event_id await_hotkey_event();

    void post_hotkey_event(Event_id::Event_id event);
    void post_ui_event();
    void post_interrupt(QString message = {});

    void load_script(const std::string &path);
    static void launch_editor(QString path, int error_line = 1);
    void launch_editor() const;
    static std::string to_string(double d);
    static std::string to_string(const sol::object &o);
    static std::string to_string(const sol::stack_proxy &object);
    QString get_absolute_filename(QString file_to_open);

    sol::table create_table();

    private: //note: most of these things are private so that the GUI thread does not access anything important. Do not make things public.
    QStringList get_string_list(const QString &name);
    std::vector<DeviceRequirements> get_device_requirement_list(const QString &name);
    void run(std::vector<MatchedDevice> &devices);
    template <class ReturnType, class... Arguments>
    ReturnType call(const char *function_name, Arguments &&... args);
    void set_error_line(const sol::error &error);

    std::unique_ptr<sol::state> lua{};
    QString path_m{};
    int error_line{0};
    UI_container *parent{nullptr};
    QPlainTextEdit *console{nullptr};
    Data_engine *data_engine{nullptr};
    QString data_engine_auto_dump_path;
    QString additional_pdf_path;
    QString data_engine_pdf_template_path;

    TestRunner *owner{nullptr};

    std::mutex await_mutex;
    std::condition_variable await_condition_variable;
    Event_id::Event_id await_condition = Event_id::invalid;

    friend void script_setup(sol::state &lua, const std::string &path, ScriptEngine &script_engine);
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

#endif // SCRIPTENGINE_H
