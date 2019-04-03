#include "testrunner.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "console.h"
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
	: console_pointer{std::make_unique<Console>([] {
        auto console = new QPlainTextEdit();
        console->setReadOnly(true);
        console->setMaximumBlockCount(1000);
		console->setVisible(false);
		return console;
	}())}
    , lua_ui_container(new UI_container(MainWindow::mw))
	, script_pointer{std::make_unique<ScriptEngine>(this->obj(), lua_ui_container, *console_pointer, this, description.get_name())}
	, script{*script_pointer}
	, name(description.get_name())
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
	MainWindow::mw->execute_in_gui_thread([this] { console.note() << "Script interrupted"; });
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

Sol_table TestRunner::get_device_requirements_table() {
	return Utility::promised_thread_call(this, [this] { return script.get_device_requirements_table(); });
}

UI_container *TestRunner::get_lua_ui_container() const {
    return lua_ui_container;
}

void TestRunner::run_script(std::vector<MatchedDevice> devices, DeviceWorker &device_worker) {
	device_worker_pointer = &device_worker;
	Utility::thread_call(this, [ this, devices = std::move(devices), &device_worker ]() mutable {
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
			MainWindow::mw->execute_in_gui_thread([ this, message = std::string{e.what()} ] { console.error() << message; });
        }
		device_worker_pointer = nullptr;
        for (auto &dev_prot : devices) {
            device_worker.set_currently_running_test(dev_prot.device, "");
        }
		for (auto &extra_device : extra_devices) {
			device_worker.set_currently_running_test(extra_device, "");
		}
        moveToThread(MainWindow::gui_thread);
        thread.quit();
		MainWindow::mw->execute_in_gui_thread([this] { console.note() << "Script stopped"; });
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

bool TestRunner::adopt_device(const MatchedDevice &device) {
	assert(currently_in_gui_thread()); //only gui thread may move devices to avoid races
	assert(not device.device->is_in_use());
	const auto dw = device_worker_pointer.load();
	if (dw) {
		dw->set_currently_running_test(device.device, name);
		extra_devices.push_back(device.device);
		return true;
	}
	return false;
}
