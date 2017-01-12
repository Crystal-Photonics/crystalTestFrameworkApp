#include "mainwindow.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "pathsettingswindow.h"
#include "plot.h"
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

MainWindow *MainWindow::mw = nullptr;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, worker(std::make_unique<Worker>(this))
	, ui(new Ui::MainWindow) {
	detail::gui_thread = QThread::currentThread();
	mw = this;
	ui->setupUi(this);
	worker->moveToThread(&worker_thread);
	worker_thread.start();
	Console::console = ui->console_edit;
	Console::mw = this;
	ui->update_devices_list_button->click();
	load_scripts();
}

MainWindow::~MainWindow() {
	worker_thread.quit();
	worker_thread.requestInterruption();
	worker_thread.wait();
	QApplication::processEvents();
	tests.clear();
	QApplication::processEvents();
	delete ui;
}

void MainWindow::align_columns() {
	Utility::thread_call(this, [this] {
		for (int i = 0; i < ui->devices_list->columnCount(); i++) {
			ui->devices_list->resizeColumnToContents(i);
		}
		for (int i = 0; i < ui->tests_list->columnCount(); i++) {
			ui->tests_list->resizeColumnToContents(i);
		}
	});
}

void MainWindow::remove_device_entry(QTreeWidgetItem *item) {
	Utility::thread_call(this, [this, item] { delete ui->devices_list->takeTopLevelItem(ui->devices_list->indexOfTopLevelItem(item)); });
}

void MainWindow::forget_device() {
	Utility::thread_call(this, [this] {
		auto selected_device_item = ui->devices_list->currentItem();
		worker->forget_device(selected_device_item);
	});
}

void MainWindow::debug_channel_codec_state(std::list<DeviceProtocol> &protocols) {
	Utility::thread_call(this, [this, protocols] {
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
	});
}

void MainWindow::load_scripts() {
	Utility::thread_call(this, [this] {
		QDirIterator dit{QSettings{}.value(Globals::test_script_path_settings_key, "").toString(), QStringList{} << "*.lua", QDir::Files};
		while (dit.hasNext()) {
			const auto &file_path = dit.next();
			tests.push_back({ui->tests_list, file_path});
		}
	});
}

void MainWindow::add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *comport) {
	Utility::thread_call(this, [this, item, tab_name, comport] {
		ui->devices_list->addTopLevelItem(item);
		align_columns();

		auto console = new QPlainTextEdit(ui->tabWidget);
		console->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
		console->setReadOnly(true);
		ui->tabWidget->addTab(console, tab_name);
		worker->connect_to_device_console(console, comport);
	});
}

void MainWindow::append_html_to_console(QString text, QPlainTextEdit *console) {
	Utility::thread_call(this, [this, text, console] { console->appendHtml(text); });
}

static std::map<int, Plot> lua_plots;

void MainWindow::create_plot(int id, QSplitter *splitter) {
	Utility::thread_call(this, [this, id, splitter] { lua_plots.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(splitter)).second; });
}

void MainWindow::add_data_to_plot(int id, double x, double y) {
	Utility::thread_call(this, [id, x, y] { lua_plots.at(id).add(x, y); });
}

void MainWindow::clear_plot(int id) {
	Utility::thread_call(this, [this, id] { lua_plots.at(id).clear(); });
}

void MainWindow::drop_plot(int id) {
	//The script will not update this plot anymore, but we keep it around so the user can save data or something
}

void MainWindow::on_actionPaths_triggered() {
	Utility::thread_call(this, [this] {
		path_dialog = new PathSettingsWindow(this);
		path_dialog->show();
	});
}

void MainWindow::on_device_detect_button_clicked() {
	Utility::thread_call(this, [this] { worker->detect_devices(); });
}

void MainWindow::on_update_devices_list_button_clicked() {
	Utility::thread_call(this, [this] {
		assert(currently_in_gui_thread());
		worker->update_devices();
	});
}

void MainWindow::on_tabWidget_tabCloseRequested(int index) {
	Utility::thread_call(this, [this, index] {
		if (ui->tabWidget->tabText(index) == "Console") {
			Console::note() << tr("Cannot close console window");
			return;
		}
		ui->tabWidget->removeTab(index);
	});
}

MainWindow::Test::Test(QTreeWidget *test_list, const QString &file_path)
	: parent(test_list)
	, test_console_widget(new QSplitter(Qt::Vertical))
	, script(test_console_widget)
	, file_path(file_path) {
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
		MainWindow::mw->test_console_remove(this);
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
	auto t = std::tie(this->parent, this->ui_item, this->script, this->protocols, this->name, this->console, this->test_console_widget, this->file_path);
	auto o = std::tie(other.parent, other.ui_item, other.script, other.protocols, other.name, other.console, other.test_console_widget, other.file_path);

	std::swap(t, o);
}

bool MainWindow::Test::operator==(QTreeWidgetItem *item) {
	return item == ui_item;
}

void MainWindow::on_run_test_script_button_clicked() {
	Utility::thread_call(this, [this] {
		auto items = ui->tests_list->selectedItems();
		for (auto &item : items) {
			auto test = Utility::from_qvariant<MainWindow::Test>(item->data(0, Qt::UserRole));
			if (test == nullptr) {
				continue;
			}
			if (test->protocols.empty()) {
				Console::error(test->console)
					<< tr("The selected script \"%1\" cannot be run, because it did not report the required devices.").arg(test->name);
			}
			for (auto &protocol : test->protocols) {
				std::promise<std::vector<ComportDescription *>> promise; //TODO: do not only loop over comport_devices, but other devices as well
				auto future = promise.get_future();
				worker->get_devices_with_protocol(protocol, promise);
				std::vector<ComportDescription *> candidates = future.get();

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
									Utility::replace_tab_widget(ui->test_tabs, 0, ui->test_tab_placeholder, tr("Test Information"));
									ui->test_tabs->setCurrentIndex(ui->test_tabs->addTab(test->test_console_widget, test->name));
									worker->run_script(&test->script, test->console, &device);
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
	});
}

void MainWindow::on_tests_list_itemClicked(QTreeWidgetItem *item, int column) {
	(void)column;
	Utility::thread_call(this, [this, item] {
		auto test = Utility::from_qvariant<MainWindow::Test>(item->data(0, Qt::UserRole));
		test_console_focus(test);
	});
}

void MainWindow::on_tests_list_customContextMenuRequested(const QPoint &pos) {
	Utility::thread_call(this, [this, pos] {
		auto item = ui->tests_list->itemAt(pos);
		if (item) {
			auto test = get_test_from_ui();
			test_console_focus(test);

			QMenu menu(this);

			QAction action_run_abort;
			switch (worker->get_state(test->script)) {
				case ScriptEngine::State::running:
					action_run_abort.setText(tr("Abort"));
					connect(&action_run_abort, &QAction::triggered, [ this, &script = test->script ] { worker->abort_script(script); });
					menu.addAction(&action_run_abort);
					break;
				case ScriptEngine::State::idle:
					action_run_abort.setText(tr("Run"));
					connect(&action_run_abort, &QAction::triggered, [this] { on_run_test_script_button_clicked(); });
					menu.addAction(&action_run_abort);
					break;
				case ScriptEngine::State::aborting:
					break;
			}

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
					tests.push_back({ui->tests_list, file_path});
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
	});
}

void MainWindow::on_devices_list_customContextMenuRequested(const QPoint &pos) {
	Utility::thread_call(this, [this, pos] {
		auto item = ui->devices_list->itemAt(pos);
		if (item) {
			QMenu menu(this);

			QAction action_detect(tr("Detect"));
			connect(&action_detect, &QAction::triggered, [this, item] { worker->detect_device(item); });
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
	});
}

MainWindow::Test *MainWindow::get_test_from_ui(const QTreeWidgetItem *item) {
	if (item == nullptr) {
		item = ui->tests_list->currentItem();
	}
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

void MainWindow::test_console_add(MainWindow::Test *test) {
	//ui->test_tabs->addTab(test->test_console_widget, test->name);
}

void MainWindow::test_console_remove(Test *test) {
	//ui->test_tabs->removeTab(ui->test_tabs->indexOf(test->test_console_widget));
}

void MainWindow::test_console_focus(Test *test) {
	//ui->test_tabs->setCurrentIndex(ui->test_tabs->indexOf(test->test_console_widget));
	auto index = ui->test_tabs->indexOf(test->test_console_widget);
	if (index == -1) { //not showing this widget right now
		Utility::replace_tab_widget(ui->test_tabs, 0, test->test_console_widget, test->name);
		index = 0;
	}
	ui->test_tabs->setCurrentIndex(index);
}

QThread *detail::gui_thread = nullptr;

bool currently_in_gui_thread() {
	return QThread::currentThread() == detail::gui_thread;
}
