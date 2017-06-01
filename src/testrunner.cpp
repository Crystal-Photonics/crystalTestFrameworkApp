#include "testrunner.h"
#include "Windows/mainwindow.h"
#include "console.h"
#include "deviceworker.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"
#include "ui_container.h"

#include <QApplication>
#include <QDebug>
#include <QPlainTextEdit>
#include <QSplitter>

TestRunner::TestRunner(const TestDescriptionLoader &description)
    : console([] {
        auto console = new QPlainTextEdit();
        console->setReadOnly(true);
        console->setMaximumBlockCount(1000);
        return console;
    }())
    , lua_ui_container(new UI_container(MainWindow::mw))
	, script(this, lua_ui_container, console, data_engine.get())
    , name(description.get_name()) {
    Console::note(console) << "Script started";
    lua_ui_container->add(console, nullptr);
    moveToThread(&thread);
    thread.start();
    try {
        script.load_script(description.get_filepath());
    } catch (const std::runtime_error &e) {
        thread.quit();
        throw;
    }
}

TestRunner::~TestRunner() {
    join();
}

void TestRunner::interrupt() {
    Utility::thread_call(this, [this]() mutable { this->script.event_queue_interrupt(); });

    MainWindow::mw->execute_in_gui_thread([this] { Console::note(console) << "Script interrupted"; });
    thread.exit(-1);
    thread.exit(-1);
    thread.requestInterruption();
}

void TestRunner::join() {
    while (!thread.wait(16)) {
        QApplication::processEvents();
    }
}

sol::table TestRunner::create_table() {
    return Utility::promised_thread_call(this, [this] { return script.create_table(); });
}

UI_container *TestRunner::get_lua_ui_container() const {
    return lua_ui_container;
}

void TestRunner::run_script(std::vector<std::pair<CommunicationDevice *, Protocol *>> devices, DeviceWorker &device_worker) {
    Utility::thread_call(this, [ this, devices = std::move(devices), &device_worker ]() mutable {
        for (auto &dev_prot : devices) {
            device_worker.set_currently_running_test(dev_prot.first, name);
        }
        try {
            script.run(devices);
        } catch (const std::runtime_error &e) {
            MainWindow::mw->execute_in_gui_thread([ this, message = std::string{e.what()} ] { Console::error(console) << message; });
        }
        for (auto &dev_prot : devices) {
            device_worker.set_currently_running_test(dev_prot.first, "");
        }
        moveToThread(MainWindow::gui_thread);
        thread.quit();
        MainWindow::mw->execute_in_gui_thread([this] { Console::note(console) << "Script stopped"; });
    });
}

bool TestRunner::is_running() {
    return thread.isRunning();
}

const QString &TestRunner::get_name() const {
    return name;
}

void TestRunner::launch_editor() const {
    script.launch_editor();
}
