#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "sol.hpp"

#include <QDebug>
#include <QEventLoop>
#include <QList>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class CommunicationDevice;
class QStringList;
class QSplitter;
class QPlainTextEdit;
struct Protocol;
class Data_engine;
class UI_container;

QString get_absolute_file_path(const QString &script_path, const QString &file_to_open);
std::string get_absolute_file_path(const QString &script_path, const std::string &file_to_open);

class DeviceRequirements {
    public:
    QString protocol_name;
    QStringList device_names;
    int quantity_min = 0;
    int quantity_max = INT_MAX;
    QString get_description() const;
};

namespace HotKeyEvent {
    enum HotKeyEvent { confirm_pressed, skip_pressed, cancel_pressed, interrupted=-1 };
}

namespace TimerEvent {
    enum TimerEvent { expired, interrupted=-1 };
}

namespace UiEvent {
    enum UiEvent { activated, interrupted=-1 };
}

class ScriptEngine {
    public:
    friend class TestRunner;
    friend class TestDescriptionLoader;
    friend class DeviceWorker;

    ScriptEngine(UI_container *parent, QPlainTextEdit *console, Data_engine *data_engine);
    ~ScriptEngine();

    TimerEvent::TimerEvent timer_event_queue_run(int timeout_ms);
    UiEvent::UiEvent ui_event_queue_run();
    HotKeyEvent::HotKeyEvent hotkey_event_queue_run();

    void hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent event);
    void ui_event_queue_send();
    void event_queue_interrupt();

    void load_script(const QString &path);
    static void launch_editor(QString path, int error_line = 1);
    void launch_editor() const;
    static std::string to_string(double d);
    static std::string to_string(const sol::object &o);
    static std::string to_string(const sol::stack_proxy &object);
    QString get_absolute_filename(QString file_to_open);

    private: //note: most of these things are private so that the GUI thread does not access anything important. Do not make things public.
    sol::table create_table();
    QStringList get_string_list(const QString &name);
    std::vector<DeviceRequirements> get_device_requirement_list(const QString &name);
    void run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices);
    template <class ReturnType, class... Arguments>
    ReturnType call(const char *function_name, Arguments &&... args);
    void set_error(const sol::error &error);

    std::unique_ptr<sol::state> lua{};
    QString path{};
    int error_line{0};
    UI_container *parent{nullptr};
    QPlainTextEdit *console{nullptr};
    Data_engine *data_engine{nullptr};
    std::unique_ptr<std::string> pdf_filepath{std::make_unique<std::string>()};
    std::unique_ptr<std::string> form_filepath{std::make_unique<std::string>()};

    QEventLoop event_loop;

    int event_queue_run_();
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
