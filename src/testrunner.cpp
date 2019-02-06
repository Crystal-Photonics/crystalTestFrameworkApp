#include "testrunner.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "console.h"
#include "data_engine/data_engine.h"
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
	, data_engine{std::make_unique<Data_engine>()}
	, script_pointer{std::make_unique<ScriptEngine>(this->obj(), lua_ui_container, console, data_engine.get())}
	, script{*script_pointer}
	, name(description.get_name()) {
    Console::note(console) << "Script started";

    lua_ui_container->add(console, nullptr);
    assert(console);
    console->setVisible(false);
    thread.adopt(*this);
    thread.start();
    try {
        script.load_script(description.get_filepath().toStdString());
    } catch (const std::runtime_error &e) {
        thread.quit();
        throw;
    }
}

TestRunner::~TestRunner() {
    message_queue_join();
}

void TestRunner::interrupt() {
    MainWindow::mw->execute_in_gui_thread([this] { Console::note(console) << "Script interrupted"; });
    script.post_interrupt();
    thread.requestInterruption();
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

UI_container *TestRunner::get_lua_ui_container() const {
    return lua_ui_container;
}

void TestRunner::run_script(std::vector<MatchedDevice> devices, DeviceWorker &device_worker) {
	Utility::thread_call(this, [ this, devices = std::move(devices), &device_worker ]() mutable {
		data_engine->enable_logging(console, devices);
        for (auto &dev_prot : devices) {
            device_worker.set_currently_running_test(dev_prot.device, name);
        }
        try {
			MainWindow::mw->execute_in_gui_thread([this] { MainWindow::mw->set_testrunner_state(this, TestRunner_State::running); });
            script.run(devices);
			MainWindow::mw->execute_in_gui_thread([this] { MainWindow::mw->set_testrunner_state(this, TestRunner_State::finished); });
        } catch (const std::runtime_error &e) {
			MainWindow::mw->execute_in_gui_thread([this] { MainWindow::mw->set_testrunner_state(this, TestRunner_State::error); });
            qDebug() << "runtime_error caught @TestRunner::run_script";
			MainWindow::mw->execute_in_gui_thread([ this, message = std::string{e.what()} ] {
                assert(console);
                Console::error(console) << message;
            });
        }
        for (auto &dev_prot : devices) {
            device_worker.set_currently_running_test(dev_prot.device, "");
        }
        moveToThread(MainWindow::gui_thread);
        thread.quit();
        MainWindow::mw->execute_in_gui_thread([this] { Console::note(console) << "Script stopped"; });
    });
}

bool TestRunner::is_running() const {
    return thread.isRunning();
}

const QString &TestRunner::get_name() const {
    return name;
}

void TestRunner::launch_editor() const {
    script.launch_editor();
}

QObject *TestRunner::obj() {
    return this;
}
