#include "mainwindow.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "LuaUI/plot.h"
#include "LuaUI/window.h"
#include "Protocols/rpcprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "config.h"
#include "console.h"
#include "deviceworker.h"
#include "pathsettingswindow.h"
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
    QTimer::singleShot(16, device_worker.get(), &DeviceWorker::poll_ports);
    Console::console = ui->console_edit;
    Console::mw = this;
    ui->update_devices_list_button->click();
    load_scripts();
}

MainWindow::~MainWindow() {
    for (auto &test : test_runners) {
        test->interrupt();
    }
    for (auto &test : test_runners) {
        test->join();
    }
    QApplication::processEvents();
    test_runners.clear();
    QApplication::processEvents();
    devices_thread.quit();
    devices_thread.wait();
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
        const auto dir = QSettings{}.value(Globals::test_script_path_settings_key, "").toString();
        QDirIterator dit{dir, QStringList{} << "*.lua", QDir::Files, QDirIterator::Subdirectories};
        while (dit.hasNext()) {
            const auto &file_path = dit.next();
            test_descriptions.push_back({ui->tests_list, file_path, QDir{dir}.relativeFilePath(file_path)});
        }
    });
}

void MainWindow::add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *comport) {
    Utility::thread_call(this, [this, item, tab_name, comport] {
        ui->devices_list->addTopLevelItem(item);
        align_columns();

        auto console = new QPlainTextEdit(ui->console_tabs);
        console->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        console->setReadOnly(true);
        console->setMaximumBlockCount(1000);
        ui->console_tabs->addTab(console, tab_name);
        device_worker->connect_to_device_console(console, comport);
    });
}

void MainWindow::append_html_to_console(QString text, QPlainTextEdit *console) {
    Utility::thread_call(this, [this, text, console] {
        if (console) {
            console->appendHtml(text);
        } else {
            Console::debug() << text;
        }
    });
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

void MainWindow::remove_test_runner(TestRunner *runner) {
    test_runners.erase(std::find(std::begin(test_runners), std::end(test_runners), runner));
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

void MainWindow::on_console_tabs_tabCloseRequested(int index) {
    Utility::thread_call(this, [this, index] {
        if (ui->console_tabs->tabText(index) == "Console") {
            Console::note() << tr("Cannot close console window");
            return;
        }
        ui->console_tabs->removeTab(index);
    });
}

static Utility::Optional<std::vector<std::pair<CommunicationDevice *, Protocol *>>> get_script_communication(DeviceWorker &device_worker, TestRunner &runner,
                                                                                                             TestDescriptionLoader &test) {
    std::vector<std::pair<CommunicationDevice *, Protocol *>> devices;
    //std::vector<ComportDescription *> candidates = device_worker.get_devices_with_protocol(protocol.protocol_name);

    if (test.get_protocols().size() ) {
    }

    ///if (test.get_protocols().size() == runner.size()) {

    //  } else {
    for (auto &protocol : test.get_protocols()) {
        //TODO: do not only loop over comport_devices, but other devices as well
        std::vector<ComportDescription *> candidates = device_worker.get_devices_with_protocol(protocol.protocol_name, protocol.device_names);
        //TODO: skip over candidates that are already in use

        //if
        switch (candidates.size()) {
            case 0:
                //failed to find suitable device
                Console::error(runner.console) << QObject::tr(
                                                      "The selected test \"%1\" requires a device with protocol \"%2\", but no such device is available.")
                                                      .arg(test.get_name(), protocol.protocol_name);
                return {};
            case 1:
                //found the only viable option
                {
                    auto &device = *candidates.front();
                    auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol.get());
                    auto scpi_protocol = dynamic_cast<SCPIProtocol *>(device.protocol.get());
                    if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
                        sol::optional<std::string> message;
                        try {
                            sol::table table = runner.create_table();
                            rpc_protocol->get_lua_device_descriptor(table);
                            message = runner.call<sol::optional<std::string>>("RPC_acceptable", std::move(table));
                        } catch (const sol::error &e) {
                            const auto &message = QObject::tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
                            Console::error(runner.console) << message;
                            return {};
                        }
                        if (message) {
                            //device incompatible, reason should be inside of message
                            Console::note(runner.console) << QObject::tr("Device rejected:") << message.value();
                            return {};
                        } else {
                            //acceptable device found
                            devices.emplace_back(device.device.get(), device.protocol.get());
                        }
                    } else if (scpi_protocol) {
                        sol::optional<std::string> message;
                        try {
                            sol::table table = runner.create_table();
                            scpi_protocol->get_lua_device_descriptor(table);
                            message = runner.call<sol::optional<std::string>>("SCPI_acceptable", std::move(table));
                        } catch (const sol::error &e) {
                            const auto &message = QObject::tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
                            Console::error(runner.console) << message;
                            return {};
                        }
                        if (message) {
                            //device incompatible, reason should be inside of message
                            Console::note(runner.console) << QObject::tr("Device rejected:") << message.value();
                            return {};
                        } else {
                            //acceptable device found
                            devices.emplace_back(device.device.get(), device.protocol.get());
                        }
                    } else {
                        assert(!"TODO: handle non-RPC protocol");
                    }
                }
                break;
            default:
                //found multiple viable options
                QMessageBox::critical(MainWindow::mw, "TODO", "TODO: implementation for multiple viable device options");
                return {};
        }
        //  }
    }
    return devices;
}

void MainWindow::on_run_test_script_button_clicked() {
    Utility::thread_call(this, [this] {
        auto items = ui->tests_list->selectedItems();
        for (auto &item : items) {
            auto test = Utility::from_qvariant<TestDescriptionLoader>(item->data(0, Qt::UserRole));
            if (test == nullptr) {
                continue;
            }
            try {
                test_runners.push_back(std::make_unique<TestRunner>(*test));
            } catch (const std::runtime_error &e) {
                Console::error(test->console) << "Failed running test: " << e.what();
                continue;
            }
            auto &runner = *test_runners.back();
            ui->test_tabs->setCurrentIndex(ui->test_tabs->addTab(runner.get_lua_ui_container(), test->get_name()));
            auto devices = get_script_communication(*device_worker, runner, *test);
            if (devices) {
                runner.run_script(devices.value(), *device_worker);
            } else {
                runner.interrupt();
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
            while (ui->tests_list->indexOfTopLevelItem(item) == -1) {
                item = item->parent();
            }

            emit on_tests_list_itemClicked(item, 0);

            auto test = get_test_from_ui();

            QMenu menu(this);

            QAction action_run(tr("Run"), nullptr);
            connect(&action_run, &QAction::triggered, [this] { on_run_test_script_button_clicked(); });
            menu.addAction(&action_run);

            QAction action_reload(tr("Reload"), nullptr);
            connect(&action_reload, &QAction::triggered, [test] { test->reload(); });
            menu.addAction(&action_reload);

            QAction action_editor(tr("Open in Editor"), nullptr);
            connect(&action_editor, &QAction::triggered, [test] { test->launch_editor(); });
            menu.addAction(&action_editor);

            menu.exec(ui->tests_list->mapToGlobal(pos));
        } else {
            QMenu menu(this);

            QAction action(tr("Reload all scripts"), nullptr);
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

        QAction action_detect(tr("Detect"), nullptr);
        connect(&action_detect, &QAction::triggered, [this, item] { device_worker->detect_device(item); });
        menu.addAction(&action_detect);

        QAction action_forget(tr("Forget"), nullptr);
        if (item->text(3).isEmpty() == false) {
            action_forget.setDisabled(true);
        } else {
            connect(&action_forget, &QAction::triggered, this, &MainWindow::forget_device);
        }
        menu.addAction(&action_forget);

        menu.exec(ui->devices_list->mapToGlobal(pos));
    } else {
        QMenu menu(this);

        QAction action_update(tr("Update device list"), nullptr);
        connect(&action_update, &QAction::triggered, ui->update_devices_list_button, &QPushButton::clicked);
        menu.addAction(&action_update);

        QAction action_detect(tr("Detect device protocols"), nullptr);
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
    QApplication::processEvents();
    test_runners.erase(runner_it);
    QApplication::processEvents();
    ui->test_tabs->removeTab(index);
}

void MainWindow::on_test_tabs_customContextMenuRequested(const QPoint &pos) {
    auto tab_index = ui->test_tabs->tabBar()->tabAt(pos);
    auto runner = get_runner_from_tab_index(tab_index);
    if (runner) {
        QMenu menu(this);

        QAction action_abort_script(tr("Abort Script"), nullptr);
        if (runner->is_running()) {
            connect(&action_abort_script, &QAction::triggered, [this, runner] {
                runner->interrupt();
                runner->join();
            });
            menu.addAction(&action_abort_script);
        }

        QAction action_open_script_in_editor(tr("Open in Editor"), nullptr);
        connect(&action_open_script_in_editor, &QAction::triggered, [this, runner] { runner->launch_editor(); });
        menu.addAction(&action_open_script_in_editor);

        QAction action_pop_out(tr("Open in extra Window"), nullptr);
        connect(&action_pop_out, &QAction::triggered, [this, runner] {
            auto container = runner->get_lua_ui_container();
            ui->test_tabs->removeTab(ui->test_tabs->indexOf(container));
            new Window(runner);
        });
        menu.addAction(&action_pop_out);

        menu.exec(ui->test_tabs->mapToGlobal(pos));
    } else {
        //clicked on the overview list
        QMenu menu(this);

        QAction action_close_finished(tr("Close finished Tests"), nullptr);
        connect(&action_close_finished, &QAction::triggered, [this] {
            test_runners.erase(std::remove_if(std::begin(test_runners), std::end(test_runners),
                                              [this](const auto &test) {
                                                  if (test->is_running()) {
                                                      return false;
                                                  }
                                                  auto container = test->get_lua_ui_container();
                                                  ui->test_tabs->removeTab(ui->test_tabs->indexOf(container));
                                                  return true;
                                              }),
                               std::end(test_runners));
        });
        menu.addAction(&action_close_finished);

        menu.exec(ui->test_tabs->mapToGlobal(pos));
    }
}

void MainWindow::on_use_human_readable_encoding_toggled(bool checked) {
    Console::use_human_readable_encoding = checked;
}

void MainWindow::on_console_tabs_customContextMenuRequested(const QPoint &pos) {
    auto tab_index = ui->console_tabs->tabBar()->tabAt(pos);
    ui->console_tabs->setCurrentIndex(tab_index);
    auto widget = ui->console_tabs->widget(tab_index);
    QPlainTextEdit *edit = dynamic_cast<QPlainTextEdit *>(widget);
    if (edit == nullptr) {
        auto &children = widget->children();
        for (auto &child : children) {
            if ((edit = dynamic_cast<QPlainTextEdit *>(child))) {
                break;
            }
        }
        assert(edit);
    }

    QMenu menu(this);

    QAction action_clear(tr("Clear"), nullptr);
    connect(&action_clear, &QAction::triggered, edit, &QPlainTextEdit::clear);
    menu.addAction(&action_clear);

    menu.exec(ui->console_tabs->mapToGlobal(pos));
}
