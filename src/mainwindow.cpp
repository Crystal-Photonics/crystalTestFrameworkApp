#include "mainwindow.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "deviceworker.h"
#include "pathsettingswindow.h"
#include "plot.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"
#include "testrunner.h"
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
	static std::map<int, Plot> lua_plots;
	struct Button {
		Button(QPushButton *button, std::function<void()> callback)
			: button(button)
			, callback(std::move(callback)) {
			connection = QObject::connect(button, &QPushButton::pressed, this->callback);
		}
		~Button() {
			QObject::disconnect(connection);
			button->setEnabled(false);
		}

		QPushButton *button = nullptr;
		std::function<void()> callback;
		QMetaObject::Connection connection;
	};

	static std::map<int, Button> lua_buttons;
}

using namespace std::chrono_literals;

MainWindow *MainWindow::mw = nullptr;

QThread *MainWindow::gui_thread;

bool currently_in_gui_thread() {
	return QThread::currentThread() == MainWindow::gui_thread;
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, device_worker(std::make_unique<DeviceWorker>())
	, ui(new Ui::MainWindow) {
	MainWindow::gui_thread = QThread::currentThread();
	mw = this;
	ui->setupUi(this);
	device_worker->moveToThread(&devices_thread);
	devices_thread.start();
	Console::console = ui->console_edit;
	Console::mw = this;
	ui->update_devices_list_button->click();
	load_scripts();
}

MainWindow::~MainWindow() {
	devices_thread.quit();
	devices_thread.wait();
	for (auto &test : test_runners) {
		test->interrupt();
	}
	for (auto &test : test_runners) {
		test->join();
	}
	QApplication::processEvents();
	test_runners.clear();
	GUI::lua_plots.clear();
	GUI::lua_buttons.clear();
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
		if (!selected_device_item) {
			return;
		}
		while (ui->devices_list->indexOfTopLevelItem(selected_device_item) == -1) {
			selected_device_item = selected_device_item->parent();
		}
		device_worker->forget_device(selected_device_item);
		delete ui->devices_list->takeTopLevelItem(ui->devices_list->indexOfTopLevelItem(selected_device_item));
	});
}

void MainWindow::load_scripts() {
	Utility::thread_call(this, [this] {
		QDirIterator dit{QSettings{}.value(Globals::test_script_path_settings_key, "").toString(), QStringList{} << "*.lua", QDir::Files};
		while (dit.hasNext()) {
			const auto &file_path = dit.next();
			test_descriptions.push_back({ui->tests_list, file_path});
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
		device_worker->connect_to_device_console(console, comport);
	});
}

void MainWindow::append_html_to_console(QString text, QPlainTextEdit *console) {
	Utility::thread_call(this, [this, text, console] { console->appendHtml(text); });
}

void MainWindow::plot_create(int id, QSplitter *splitter) {
	Utility::thread_call(this,
						 [this, id, splitter] { GUI::lua_plots.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(splitter)).second; });
}

void MainWindow::plot_add_data(int id, double x, double y) {
	Utility::thread_call(this, [id, x, y] { GUI::lua_plots.at(id).add(x, y); });
}

void MainWindow::plot_add_data(int id, const std::vector<double> &data) {
	Utility::thread_call(this, [id, data] { GUI::lua_plots.at(id).add(data); });
}

void MainWindow::plot_add_data(int id, const unsigned int spectrum_start_channel, const std::vector<double> &data) {
	Utility::thread_call(this, [id, spectrum_start_channel, data] { GUI::lua_plots.at(id).add(spectrum_start_channel, data); });
}

void MainWindow::plot_clear(int id) {
	Utility::thread_call(this, [this, id] { GUI::lua_plots.at(id).clear(); });
}

void MainWindow::plot_drop(int id) {
	Utility::thread_call(this, [id] { GUI::lua_plots.erase(id); });
}

void MainWindow::plot_set_offset(int id, double offset) {
	Utility::thread_call(this, [id, offset] { GUI::lua_plots.at(id).set_offset(offset); });
}

void MainWindow::plot_set_gain(int id, double gain) {
	Utility::thread_call(this, [id, gain] { GUI::lua_plots.at(id).set_gain(gain); });
}

void MainWindow::button_create(int id, QSplitter *splitter, const std::string &title, std::function<void()> callback) {
	Utility::thread_call(this, [ this, id, splitter, title, callback = std::move(callback) ]() mutable {
		auto button = new QPushButton(title.c_str(), splitter);
		splitter->addWidget(button);
		GUI::lua_buttons.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(button, std::move(callback)));
	});
}

void MainWindow::button_drop(int id) {
	Utility::thread_call(this, [this, id] { GUI::lua_buttons.erase(id); });
}

void MainWindow::show_message_box(const QString &title, const QString &message, QMessageBox::Icon icon) {
	Utility::thread_call(this, [this, title, message, icon] {
		switch (icon) {
			default:
			case QMessageBox::Critical:
				QMessageBox::critical(this, title, message);
				break;
			case QMessageBox::Warning:
				QMessageBox::warning(this, title, message);
				break;
			case QMessageBox::Information:
				QMessageBox::information(this, title, message);
				break;
			case QMessageBox::Question:
				QMessageBox::question(this, title, message);
				break;
		}
	});
}

void MainWindow::on_actionPaths_triggered() {
	Utility::thread_call(this, [this] {
		path_dialog = new PathSettingsWindow(this);
		path_dialog->show();
	});
}

void MainWindow::on_device_detect_button_clicked() {
	Utility::thread_call(this, [this] { device_worker->detect_devices(); });
}

void MainWindow::on_update_devices_list_button_clicked() {
	Utility::thread_call(this, [this] {
		assert(currently_in_gui_thread());
		device_worker->update_devices();
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

void MainWindow::on_run_test_script_button_clicked() {
	Utility::thread_call(this, [this] {
		auto items = ui->tests_list->selectedItems();
		for (auto &item : items) {
			auto test = Utility::from_qvariant<TestDescriptionLoader>(item->data(0, Qt::UserRole));
			if (test == nullptr) {
				continue;
			}
			if (test->get_protocols().empty()) {
				Console::error(test->console)
					<< tr("The selected script \"%1\" cannot be run, because it did not report the required devices.").arg(test->get_name());
			}
			for (auto &protocol : test->get_protocols()) {
				std::promise<std::vector<ComportDescription *>> promise; //TODO: do not only loop over comport_devices, but other devices as well
				auto future = promise.get_future();
				device_worker->get_devices_with_protocol(protocol, promise);
				std::vector<ComportDescription *> candidates = future.get();

				switch (candidates.size()) {
					case 0:
						//failed to find suitable device
						Console::error(test->console) << tr("The selected test \"%1\" requires a device with protocol \"%2\", but no such device is available.")
															 .arg(test->get_name(), protocol);
						break;
					case 1:
						//found the only viable option
						{
							test_runners.push_back(std::make_unique<TestRunner>(*test));
							auto &runner = *test_runners.back();
							ui->test_tabs->setCurrentIndex(ui->test_tabs->addTab(runner.get_lua_ui_container(), test->get_name()));
							auto &device = *candidates.front();
							auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol.get());
							if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
								sol::optional<std::string> message;
								try {
									sol::table table = runner.create_table();
									rpc_protocol->get_lua_device_descriptor(table);
									message = runner.call<sol::optional<std::string>>("RPC_acceptable", std::move(table));
								} catch (const sol::error &e) {
									const auto &message = tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
									Console::error(runner.console) << message;
									runner.interrupt();
									return;
								}
								if (message) {
									//device incompatible, reason should be inside of message
									Console::note(runner.console) << tr("Device rejected:") << message.value();
									runner.interrupt();
								} else {
									//acceptable device found
									runner.run_script({{device.device.get(), device.protocol.get()}}, *device_worker);
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
		auto test = Utility::from_qvariant<TestDescriptionLoader>(item->data(0, Qt::UserRole));
		Utility::replace_tab_widget(ui->test_tabs, 0, test->console.get(), test->get_name());
		ui->test_tabs->setCurrentIndex(0);
	});
}

void MainWindow::on_tests_list_customContextMenuRequested(const QPoint &pos) {
	Utility::thread_call(this, [this, pos] {
		auto item = ui->tests_list->itemAt(pos);
		if (item) {
			auto test = get_test_from_ui();

			QMenu menu(this);

			QAction action_run(tr("Run"));
			connect(&action_run, &QAction::triggered, [this] { on_run_test_script_button_clicked(); });
			menu.addAction(&action_run);

			QAction action_reload(tr("Reload"));
			connect(&action_reload, &QAction::triggered, [test] { test->reload(); });
			menu.addAction(&action_reload);

			QAction action_editor(tr("Open in Editor"));
			connect(&action_editor, &QAction::triggered, [test] { test->launch_editor(); });
			menu.addAction(&action_editor);

			menu.exec(ui->tests_list->mapToGlobal(pos));
		} else {
			QMenu menu(this);

			QAction action(tr("Reload all scripts"));
			connect(&action, &QAction::triggered, [this] {
				test_descriptions.clear();
				load_scripts();
			});
			menu.addAction(&action);

			menu.exec(ui->tests_list->mapToGlobal(pos));
		}
	});
}

void MainWindow::on_devices_list_customContextMenuRequested(const QPoint &pos) {
	auto item = ui->devices_list->itemAt(pos);
	if (item) {
		while (ui->devices_list->indexOfTopLevelItem(item) == -1) {
			item = item->parent();
		}
		QMenu menu(this);

		QAction action_detect(tr("Detect"));
		connect(&action_detect, &QAction::triggered, [this, item] { device_worker->detect_device(item); });
		menu.addAction(&action_detect);

		QAction action_forget(tr("Forget"));
		if (item->text(3).isEmpty() == false) {
			action_forget.setDisabled(true);
		} else {
			connect(&action_forget, &QAction::triggered, this, &MainWindow::forget_device);
		}
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

TestDescriptionLoader *MainWindow::get_test_from_ui(const QTreeWidgetItem *item) {
	if (item == nullptr) {
		item = ui->tests_list->currentItem();
	}
	if (item == nullptr) {
		return nullptr;
	}
	for (auto &test : test_descriptions) {
		if (test.ui_entry == item) {
			return &test;
		}
	}
	return nullptr;
}

TestRunner *MainWindow::get_runner_from_tab_index(int index) {
	for (auto &r : test_runners) {
		auto runner_index = ui->test_tabs->indexOf(r->get_lua_ui_container());
		if (runner_index == index) {
			return r.get();
		}
	}
	return nullptr;
}

void MainWindow::on_test_tabs_tabCloseRequested(int index) {
	if (index == 0) {
		//first tab never gets closed
		return;
	}
	auto tab_widget = ui->test_tabs->widget(index);
	auto runner_it = std::find_if(std::begin(test_runners), std::end(test_runners),
								  [tab_widget](const auto &runner) { return runner->get_lua_ui_container() == tab_widget; });
	if (runner_it == std::end(test_runners)) {
		return;
	}
	auto &runner = **runner_it;
	if (runner.is_running()) {
		if (QMessageBox::question(this, tr(""), tr("Selected script %1 is still running. Abort it now?").arg(runner.get_name()),
								  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
			runner.interrupt();
			runner.join();
		} else {
			return; //canceled closing the tab
		}
	}
	ui->test_tabs->removeTab(index);
	QApplication::processEvents();
	test_runners.erase(runner_it);
}

void MainWindow::on_test_tabs_customContextMenuRequested(const QPoint &pos) {
	auto tab_index = ui->test_tabs->tabBar()->tabAt(pos);
	if (tab_index <= 0) {
		//clicked on the overview list
		QMenu menu(this);

		QAction action_close_finished(tr("Close finished Tests"));
		//connect(&action_close_finished, &QAction::triggered, [this] { QMessageBox::information(MainWindow::mw, "test", "bla"); });
		action_close_finished.setDisabled(true);
		menu.addAction(&action_close_finished);

		menu.exec(ui->test_tabs->mapToGlobal(pos));
	} else {
		auto runner = get_runner_from_tab_index(tab_index);
		assert(runner);

		QMenu menu(this);

		QAction action_abort_script(tr("Abort Script"));
		if (runner->is_running()) {
			connect(&action_abort_script, &QAction::triggered, [this, runner] {
				runner->interrupt();
				runner->join();
			});
			menu.addAction(&action_abort_script);
		}

		QAction action_open_script_in_editor(tr("Open in Editor"));
		connect(&action_open_script_in_editor, &QAction::triggered, [this, runner] { runner->launch_editor(); });
		menu.addAction(&action_open_script_in_editor);

		menu.exec(ui->test_tabs->mapToGlobal(pos));
	}
}
