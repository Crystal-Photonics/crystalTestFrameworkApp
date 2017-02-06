#include "testrunner.h"
#include "console.h"
#include "deviceworker.h"
#include "luaui.h"
#include "mainwindow.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <QSplitter>

TestRunner::TestRunner(const TestDescriptionLoader &description)
	: lua_ui_container(new QSplitter(MainWindow::mw))
	, script(lua_ui_container)
	, name(description.get_name()) {
	console = new QPlainTextEdit();
	console->setReadOnly(true);
	Console::note(console) << "Script started";
	lua_ui_container->addWidget(console);
	lua_ui_container->setOrientation(Qt::Vertical);
	moveToThread(&thread);
	thread.start();
	script.load_script(description.get_filepath());
}

TestRunner::~TestRunner() {
	join();
}

void TestRunner::interrupt() {
	MainWindow::mw->execute_in_gui_thread([this] { Console::note(console) << "Script interrupted"; });
	thread.exit(-1);
	thread.exit(-1);
	thread.requestInterruption();
}

void TestRunner::join() {
	thread.wait();
}

sol::table TestRunner::create_table() {
	return Utility::promised_thread_call(this, [this] { return script.create_table(); });
}

QSplitter *TestRunner::get_lua_ui_container() const {
	return lua_ui_container;
}

void TestRunner::run_script(std::vector<std::pair<CommunicationDevice *, Protocol *>> devices, const DeviceWorker &device_worker) {
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
