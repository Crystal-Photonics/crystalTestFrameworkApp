#include "testrunner.h"
#include "mainwindow.h"
#include "testdescriptionloader.h"

#include <QSplitter>

TestRunner::TestRunner(const TestDescriptionLoader &description)
	: lua_ui_container(new QSplitter(MainWindow::mw))
	, script(lua_ui_container) {
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

QSplitter *TestRunner::get_lua_ui_container() const
{
	return lua_ui_container;
}

void TestRunner::run_script(ComportDescription &device)
{
	throw "TODO";
}

bool TestRunner::is_running()
{
	throw "TODO";
}

QString TestRunner::get_name()
{
	throw "TODO";
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

MainWindow::Test::~Test() {
	if (ui_item != nullptr) {
		delete parent->takeTopLevelItem(parent->indexOfTopLevelItem(ui_item));
		MainWindow::mw->test_console_remove(this);
	}
}

MainWindow::Test::Test(MainWindow::Test &&other)
	: script(nullptr)
	, worker(std::move(other.worker)) {
	swap(other);
	ui_item->setData(0, Qt::UserRole, Utility::make_qvariant(this));
}

MainWindow::Test &MainWindow::Test::operator=(MainWindow::Test &&other) {
	swap(other);
	return *this;
}

void MainWindow::Test::swap(MainWindow::Test &other) {
	auto t = std::tie(this->parent, this->ui_item, this->script, this->protocols, this->name, this->console, this->test_console_widget, this->file_path,
					  this->worker, this->worker_thread);
	auto o = std::tie(other.parent, other.ui_item, other.script, other.protocols, other.name, other.console, other.test_console_widget, other.file_path,
					  other.worker, other.worker_thread);

	std::swap(t, o);
}

void MainWindow::Test::reset_ui() {
	test_console_widget = new QSplitter(Qt::Vertical);
	console = new QPlainTextEdit(test_console_widget);
	console->setReadOnly(true);
	test_console_widget->addWidget(console);
	ui_item->setData(0, Qt::UserRole, Utility::make_qvariant(this));
	worker->set_gui_parent(script, test_console_widget);
}

bool MainWindow::Test::operator==(QTreeWidgetItem *item) {
	return item == ui_item;
}

#endif
