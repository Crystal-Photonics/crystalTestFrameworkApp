#include "mainwindow.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "pathsettingswindow.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"
#include "util.h"

#include <QAction>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QListView>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <algorithm>
#include <cassert>
#include <future>
#include <iterator>
#include <memory>

namespace GUI {
	//ID's referring to the device
	namespace Devices {
		enum {
			description,
			protocol,
			name,
			current_test,
		};
	}
	namespace Tests {
		enum {
			name,
			protocol,
			deviceNames,
			connectedDevices,
		};
	}
}

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, worker(std::make_unique<Worker>(this))
	, ui(new Ui::MainWindow) {
	detail::gui_thread = QThread::currentThread();
	ui->setupUi(this);
	worker->moveToThread(&worker_thread);
	worker_thread.start();
	Console::console = ui->console_edit;
	Console::mw = this;
	ui->update_devices_list_button->click();
	load_scripts();
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::align_columns() {
	for (int i = 0; i < ui->devices_list->columnCount(); i++) {
		ui->devices_list->resizeColumnToContents(i);
	}
	for (int i = 0; i < ui->tests_list->columnCount(); i++) {
		ui->tests_list->resizeColumnToContents(i);
	}
}

void MainWindow::remove_device_entry(QTreeWidgetItem *item) {
	delete ui->devices_list->takeTopLevelItem(ui->devices_list->indexOfTopLevelItem(item));
}

void MainWindow::forget_device() {
	auto selected_device_item = ui->devices_list->currentItem();
	emit worker->forget_device(selected_device_item);
}

void MainWindow::debug_channel_codec_state(std::list<DeviceProtocol> &protocols) {
	auto test = get_test_from_ui();
	if (test == nullptr) {
		Console::debug() << "Invalid Test";
		return;
	}
	if (protocols.size() != 1) {
		Console::debug(test->console) << "Expected 1 protocol, but got" << protocols.size();
		return;
	}
	auto rpc_protocol = dynamic_cast<const RPCProtocol *>(&protocols.front().protocol);
	if (rpc_protocol == nullptr) {
		Console::debug(test->console) << "Test is not using RPC Protocol";
		return;
	}
	auto instance = rpc_protocol->debug_get_channel_codec_instance();
	if (instance == nullptr) {
		Console::debug(test->console) << "RPC Channel Codec Instance is null";
		return;
	}
	if (instance->i.initialized == false) {
		Console::debug(test->console) << "RPC Channel Codec Instance is not initialized";
		return;
	}
	auto &rx = instance->i.rxState;
	Console::debug(test->console) << "State:" << instance->i.ccChannelState                        //
								  << ',' << "rxBitmask:" << static_cast<int>(rx.bitmask)           //
								  << ',' << "Buffer Length:" << rx.bufferLength                    //
								  << ',' << "Index in Block:" << static_cast<int>(rx.indexInBlock) //
								  << ',' << "Write Pointer:" << rx.writePointer                    //
								  << ',' << "Buffer: " << QByteArray(rx.buffer).toPercentEncoding(" :\t\\\n!\"§$%&/()=+-*").toStdString();
}

void MainWindow::load_scripts() {
	QDirIterator dit{QSettings{}.value(Globals::test_script_path_settings_key, "").toString(), QStringList{} << "*.lua", QDir::Files};
	while (dit.hasNext()) {
		const auto &file_path = dit.next();
		tests.push_back({ui->tests_list, ui->test_tabs, file_path});
	}
}

void MainWindow::add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *comport) {
	ui->devices_list->addTopLevelItem(item);
	align_columns();

	auto console = new QPlainTextEdit(ui->tabWidget);
	console->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
	console->setReadOnly(true);
	ui->tabWidget->addTab(console, tab_name);
	worker->connect_to_device_console(console, comport);
}

void MainWindow::append_html_to_console(QString text, QPlainTextEdit *console) {
	console->appendHtml(text);
}

void MainWindow::on_actionPaths_triggered() {
	path_dialog = new PathSettingsWindow(this);
	path_dialog->show();
}

void MainWindow::on_device_detect_button_clicked() {
	emit worker->detect_devices();
}

void MainWindow::on_update_devices_list_button_clicked() {
	assert(currently_in_gui_thread());
	emit worker->update_devices();
}

void MainWindow::on_tabWidget_tabCloseRequested(int index) {
	if (ui->tabWidget->tabText(index) == "Console") {
		Console::note() << tr("Cannot close console window");
		return;
	}
	ui->tabWidget->removeTab(index);
}

MainWindow::Test *MainWindow::get_test_from_ui() {
	auto item = ui->tests_list->currentItem();
	if (item == nullptr) {
		return nullptr;
	}
	for (auto &test : tests) {
		if (test.ui_item == item) {
			return &test;
		}
	}
	return nullptr;
}

MainWindow::Test::Test(QTreeWidget *test_list, QTabWidget *test_tabs, const QString &file_path)
	: parent(test_list)
	, test_tabs(test_tabs)
	, splitter(new QSplitter(Qt::Vertical, test_tabs))
	, script(splitter)
	, file_path(file_path) {
	name = QString{file_path.data() + file_path.lastIndexOf('/') + 1};
	if (name.endsWith(".lua")) {
		name.chop(4);
	}
	console = new QPlainTextEdit(splitter);
	console->setReadOnly(true);
	splitter->addWidget(console);
	test_tabs->addTab(splitter, name);

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
		QStringList protocols = script.get_string_list("protocols");
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
		auto deviceNames = script.get_string_list("deviceNames");
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
	}
	if (console != nullptr) {
		test_tabs->removeTab(test_tabs->indexOf(console));
	}
}

MainWindow::Test::Test(MainWindow::Test &&other)
	: script(nullptr) {
	swap(other);
	ui_item->setData(0, Qt::UserRole, Utility::make_qvariant(this));
}

MainWindow::Test &MainWindow::Test::operator=(MainWindow::Test &&other) {
	swap(other);
	return *this;
}

void MainWindow::Test::swap(MainWindow::Test &other) {
	auto t = std::tie(this->parent, this->ui_item, this->script, this->protocols, this->name, this->test_tabs, this->console, this->file_path);
	auto o = std::tie(other.parent, other.ui_item, other.script, other.protocols, other.name, other.test_tabs, other.console, other.file_path);

	std::swap(t, o);
}

bool MainWindow::Test::operator==(QTreeWidgetItem *item) {
	return item == ui_item;
}

int MainWindow::Test::get_tab_id() const {
	return test_tabs->indexOf(static_cast<QWidget *>(console->parent()));
}

void MainWindow::Test::activate_console() {
	test_tabs->setCurrentIndex(get_tab_id());
}

void MainWindow::on_run_test_script_button_clicked() {
	auto items = ui->tests_list->selectedItems();
	for (auto &item : items) {
		auto test = Utility::from_qvariant<MainWindow::Test>(item->data(0, Qt::UserRole));
		if (test == nullptr) {
			continue;
		}
		if (test->protocols.empty()) {
			Console::error(test->console) << tr("The selected script \"%1\" cannot be run, because it did not report the required devices.").arg(test->name);
		}
		for (auto &protocol : test->protocols) {
			std::promise<std::vector<ComportDescription *>> promise; //TODO: do not only loop over comport_devices, but other devices as well
			emit worker->get_devices_with_protocol(protocol, promise);
			std::vector<ComportDescription *> candidates = promise.get_future().get();

			switch (candidates.size()) {
				case 0:
					//failed to find suitable device
					Console::error(test->console) << tr("The selected test \"%1\" requires a device with protocol \"%2\", but no such device is available.")
														 .arg(test->ui_item->text(0), protocol);
					break;
				case 1:
					//found the only viable option
					{
						auto &device = *candidates.front();
						auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol.get());
						if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
							sol::optional<std::string> message;
							try {
								sol::table table = test->script.create_table();
								rpc_protocol->get_lua_device_descriptor(table);
								message = test->script.call<sol::optional<std::string>>("RPC_acceptable", std::move(table));
							} catch (const sol::error &e) {
								const auto &message = tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
								Console::error(test->console) << message;
								return;
							}
							if (message) {
								//device incompatible, reason should be inside of message
								Console::note(test->console) << tr("Device rejected:") << message.value();
							} else {
								//acceptable device
								emit worker->run_script(&test->script, test->console, &device);
							}
						} else {
							assert(!"TODO: handle non-RPC protocol");
						}
					}
					break;
				default:
					//found multiple viable options
					QMessageBox::critical(this, "TODO", "TODO: implementation for multiple viable device options");
					break;
			}
		}
	}
}

void MainWindow::on_tests_list_itemClicked(QTreeWidgetItem *item, int column) {
	(void)column;
	auto test = Utility::from_qvariant<MainWindow::Test>(item->data(0, Qt::UserRole));
	test->activate_console();
}

void MainWindow::on_tests_list_customContextMenuRequested(const QPoint &pos) {
	auto item = ui->tests_list->itemAt(pos);
	if (item) {
		get_test_from_ui()->activate_console();

		QMenu menu(this);

		QAction action_run(tr("Run"));
		connect(&action_run, &QAction::triggered, [this] { emit on_run_test_script_button_clicked(); });
		menu.addAction(&action_run);

		QAction action_reload(tr("Reload"));
		connect(&action_reload, &QAction::triggered, [this] {
			auto selected_test_item = ui->tests_list->currentItem();
			QString file_path;
			for (auto test_it = tests.begin(); test_it != tests.end(); ++test_it) {
				if (test_it->ui_item == selected_test_item) {
					file_path = test_it->file_path;
					test_it = tests.erase(test_it);
					break;
				}
			}
			if (file_path.isEmpty() == false) {
				tests.push_back({ui->tests_list, ui->test_tabs, file_path});
			}
		});
		menu.addAction(&action_reload);

		QAction action_editor(tr("Open in Editor"));
		connect(&action_editor, &QAction::triggered, [this] { get_test_from_ui()->script.launch_editor(); });
		menu.addAction(&action_editor);

		menu.exec(ui->tests_list->mapToGlobal(pos));
	} else {
		QMenu menu(this);

		QAction action(tr("Reload all scripts"));
		connect(&action, &QAction::triggered, [this] {
			tests.clear();
			load_scripts();
		});
		menu.addAction(&action);

		menu.exec(ui->tests_list->mapToGlobal(pos));
	}
}

void MainWindow::on_devices_list_customContextMenuRequested(const QPoint &pos) {
	auto item = ui->devices_list->itemAt(pos);
	if (item) {
		QMenu menu(this);

		QAction action_detect(tr("Detect"));
		connect(&action_detect, &QAction::triggered, [this, item] { emit worker->detect_device(item); });
		menu.addAction(&action_detect);

		QAction action_forget(tr("Forget"));
		connect(&action_forget, &QAction::triggered, this, &MainWindow::forget_device);
		menu.addAction(&action_forget);

		menu.exec(ui->devices_list->mapToGlobal(pos));
	} else {
		QMenu menu(this);

		QAction action_update(tr("Update device list"));
		connect(&action_update, &QAction::triggered, ui->update_devices_list_button, &QPushButton::clicked);
		menu.addAction(&action_update);

		QAction action_detect(tr("Detect device protocols"));
		connect(&action_detect, &QAction::triggered, ui->device_detect_button, &QPushButton::clicked);
		menu.addAction(&action_detect);

		menu.exec(ui->devices_list->mapToGlobal(pos));
	}
}

QThread *detail::gui_thread = nullptr;

bool currently_in_gui_thread() {
	return QThread::currentThread() == detail::gui_thread;
}
