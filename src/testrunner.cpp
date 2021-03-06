#include "testrunner.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "Windows/plaintextedit.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h"
#include "deviceworker.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"
#include "ui_container.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>

TestRunner::TestRunner(const TestDescriptionLoader &description)
    : console_pointer{std::make_unique<Console>([] {
        auto console = new PlainTextEdit();
        console->setReadOnly(true);
        console->setMaximumBlockCount(1000);
        console->setVisible(false);
        return console;
    }())}
    , lua_ui_container(new UI_container(MainWindow::mw))
    , script_pointer{std::make_unique<ScriptEngine>(lua_ui_container, *console_pointer, this, description.get_name())}
    , script{*script_pointer}
    , name{description.get_name()}
    , script_path{QDir{QSettings{}.value(Globals::test_script_path_settings_key, "").toString()}.filePath(description.get_filepath())}
    , console{*console_pointer} {
    console.note() << "Script started";

    lua_ui_container->add(console.get_plaintext_edit(), nullptr);
    thread.adopt(*this);
    thread.start();
    try {
        script.load_script(description.get_filepath().toStdString());
    } catch (const std::runtime_error &e) {
        qDebug() << e.what();
        thread.quit();
        throw;
    }
}

TestRunner::~TestRunner() {
    message_queue_join();
}

void TestRunner::interrupt() {
    assert(currently_in_gui_thread());
    console.note() << "Script interrupted";
    script.post_interrupt();
    thread.requestInterruption();
}

bool TestRunner::was_interrupted() const {
    return thread.was_interrupted();
}

void TestRunner::message_queue_join() {
    assert(not thread.is_current());
    while (!thread.wait(16)) {
        QApplication::processEvents();
    }
}

void TestRunner::blocking_join() {
    assert(not thread.is_current());
    thread.wait();
}

Sol_table TestRunner::create_table() {
    return Utility::promised_thread_call(this, [this] { return script.create_table(); });
}

Sol_table TestRunner::get_device_requirements_table() {
    return Utility::promised_thread_call(this, [this] { return script.get_device_requirements_table(); });
}

UI_container *TestRunner::get_lua_ui_container() const {
    return lua_ui_container;
}

void TestRunner::run_script(std::vector<MatchedDevice> devices, DeviceWorker &device_worker) {
    device_worker_pointer = &device_worker;
    Utility::thread_call(this, [this, devices = std::move(devices), &device_worker]() mutable {
        for (auto &dev_prot : devices) {
            device_worker.set_currently_running_test(dev_prot.device, name);
            used_devices.push_back(dev_prot.device);
        }
        try {
            MainWindow::mw->execute_in_gui_thread([this] { MainWindow::mw->set_testrunner_state(this, TestRunner_State::running); });
            script.run(devices);
            MainWindow::mw->execute_in_gui_thread([this] { MainWindow::mw->set_testrunner_state(this, TestRunner_State::finished); });
        } catch (const std::exception &e) {
            MainWindow::mw->execute_in_gui_thread([this] { MainWindow::mw->set_testrunner_state(this, TestRunner_State::error); });
            qDebug() << "runtime_error caught @TestRunner::run_script:" << e.what() << '\n';
            console.error() << Sol_error_message{e.what(), script_path, name};
        }
        device_worker_pointer = nullptr;
        for (auto &extra_device : used_devices) {
            device_worker.set_currently_running_test(extra_device, "");
        }
        used_devices.clear();
        moveToThread(MainWindow::gui_thread);
        thread.quit();
        console.note() << "Script stopped";
    });
}

bool TestRunner::is_running() const {
    return thread.isRunning();
}

const QString &TestRunner::get_name() const {
    return name;
}

const QString &TestRunner::get_script_path() const {
    return script_path;
}

void TestRunner::launch_editor() const {
    script.launch_editor();
}

QObject *TestRunner::obj() {
    return this;
}

bool TestRunner::adopt_device(const MatchedDevice &device) {
    assert(currently_in_gui_thread()); //only gui thread may move devices to avoid races
    assert(not device.device->is_in_use());
    const auto dw = device_worker_pointer.load();
    if (dw) {
        dw->set_currently_running_test(device.device, name);
        used_devices.push_back(device.device);
        return true;
    }
    return false;
}

bool TestRunner::uses_device(CommunicationDevice *device) {
    return std::find(std::begin(used_devices), std::end(used_devices), device) != std::end(used_devices);
}
