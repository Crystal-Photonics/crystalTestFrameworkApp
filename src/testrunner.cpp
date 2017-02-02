#include "testrunner.h"
#include "console.h"
#include "mainwindow.h"
#include "testdescriptionloader.h"

#include <QSplitter>
#include <QDebug>

TestRunner::TestRunner(const TestDescriptionLoader &description)
	: lua_ui_container(new QSplitter(MainWindow::mw))
	, script(lua_ui_container)
	, name(description.get_name()) {
	lua_ui_container->setOrientation(Qt::Vertical);
	moveToThread(&thread);
	thread.start();
	script.load_script(description.get_filepath());
}

TestRunner::~TestRunner() {
	join();
}

void TestRunner::interrupt() {
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

void TestRunner::run_script(std::vector<std::pair<CommunicationDevice *, Protocol *>> devices) {
	Utility::thread_call(this, [ this, devices = std::move(devices) ]() mutable {
		try {
			script.run(devices);
		} catch (const std::runtime_error &e) {
			MainWindow::mw->execute_in_gui_thread([ this, message = std::string{e.what()} ] { Console::error(console) << message; });
		}
		moveToThread(MainWindow::gui_thread);
		thread.quit();
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

#if 0
MainWindow::Test::Test(QTreeWidget *test_list, const QString &file_path)
	: parent(test_list)
	, test_console_widget(new QSplitter(Qt::Vertical))
	, script(test_console_widget)
	, file_path(file_path)
	, worker(std::make_unique<Worker>(MainWindow::mw)) {
	name = QString{file_path.data() + file_path.lastIndexOf('/') + 1};
	if (name.endsWith(".lua")) {
		name.chop(4);
	}
	console = new QPlainTextEdit(test_console_widget);
	console->setReadOnly(true);
	test_console_widget->addWidget(console);
	MainWindow::mw->test_console_add(this);

	ui_item = new QTreeWidgetItem(test_list, QStringList{} << name);
	ui_item->setData(0, Qt::UserRole, Utility::make_qvariant(this));
	parent->addTopLevelItem(ui_item);
	try {
		script.load_script(file_path);
	} catch (const sol::error &e) {
		Console::error(console) << tr("Failed loading script \"%1\" because %2").arg(file_path, e.what());
		return;
	}
	try {
		QStringList protocols = MainWindow::mw->device_worker->get_string_list(script, "protocols");
		std::copy(protocols.begin(), protocols.end(), std::back_inserter(this->protocols));
		std::sort(this->protocols.begin(), this->protocols.end());
		if (protocols.empty() == false) {
			ui_item->setText(GUI::Tests::protocol, protocols.join(", "));
		} else {
			ui_item->setText(GUI::Tests::protocol, tr("None"));
			ui_item->setTextColor(1, Qt::darkRed);
		}
	} catch (const sol::error &e) {
		Console::error(console) << tr("Failed retrieving variable \"protocols\" from %1 because %2").arg(file_path, e.what());
		ui_item->setText(GUI::Tests::protocol, tr("Failed getting protocols"));
		ui_item->setTextColor(GUI::Tests::protocol, Qt::darkRed);
	}
	try {
		auto deviceNames = MainWindow::mw->device_worker->get_string_list(script, "deviceNames");
		if (deviceNames.isEmpty() == false) {
			ui_item->setText(GUI::Tests::deviceNames, deviceNames.join(", "));
		}
	} catch (const sol::error &e) {
		Console::warning(console) << tr("Failed retrieving variable \"deviceNames\" from %1 because %2").arg(file_path, e.what());
	}
}

#endif
