#include "mainwindow.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "CommunicationDevices/usbtmccommunicationdevice.h"
#include "LuaUI/plot.h"
#include "LuaUI/window.h"
#include "Protocols/sg04countprotocol.h"
#include "Windows/dummydatacreator.h"
#include "config.h"
#include "console.h"
#include "deviceworker.h"
#include "hotkey_picker.h"
#include "pathsettingswindow.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"
#include "testrunner.h"
#include "ui_container.h"
#include "ui_mainwindow.h"
#include "util.h"

#include "devicematcher.h"
#include <QAction>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QListView>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
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
    QTimer::singleShot(500, this, &MainWindow::poll_sg04_counts);
    Console::console = ui->console_edit;
    Console::mw = this;
    // connect(&action_run, &QAction::triggered, [this] { on_run_test_script_button_clicked(); });

    devices_thread.start();
    connect(device_worker.get(), DeviceWorker::device_discrovery_done, this, slot_device_discovery_done);
    ui->btn_refresh_all->click();
    load_scripts();
}

MainWindow::~MainWindow() {
    for (auto &test : test_runners) {
        if (test->is_running()) {
            test->interrupt();
        }
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
    assert(currently_in_gui_thread());
    for (int i = 0; i < ui->devices_list->columnCount(); i++) {
        ui->devices_list->resizeColumnToContents(i);
    }
    for (int i = 0; i < ui->tests_list->columnCount(); i++) {
        ui->tests_list->resizeColumnToContents(i);
    }
}

//void MainWindow::set_manual_devices_parent_item(std::unique_ptr<QTreeWidgetItem> manual_devices_parent_item) {
//  manual_devices_parent_item_mutex.lock();
//this->manual_devices_parent_item = manual_devices_parent_item;
// manual_devices_parent_item_mutex.unlock();
//}

std::unique_ptr<QTreeWidgetItem> *MainWindow::create_manual_devices_parent_item() {
    manual_devices_parent_item = std::make_unique<QTreeWidgetItem>(QStringList{} << "Manual Devices");
    //if (manual_parent_item->get()) {
    add_device_item(manual_devices_parent_item.get(), "test", nullptr);
    //}
    return &manual_devices_parent_item;
}

std::unique_ptr<QTreeWidgetItem> *MainWindow::MainWindow::get_manual_devices_parent_item() {
    return &manual_devices_parent_item;
}

void MainWindow::load_scripts() {
    assert(currently_in_gui_thread());
    const auto dir = QSettings{}.value(Globals::test_script_path_settings_key, "").toString();
    QDirIterator dit{dir, QStringList{} << "*.lua", QDir::Files, QDirIterator::Subdirectories};
    while (dit.hasNext()) {
        const auto &file_path = dit.next();
        test_descriptions.push_back(TestDescriptionLoader{ui->tests_list, file_path, QDir{dir}.relativeFilePath(file_path)});
    }
}

void MainWindow::add_device_child_item(QTreeWidgetItem *parent, QTreeWidgetItem *child, const QString &tab_name, CommunicationDevice *cummincation_device) {
    //called from device worker
    Utility::thread_call(this, nullptr, [this, parent, child, tab_name, cummincation_device] {
        assert(currently_in_gui_thread());
        if (parent) {
            parent->addChild(child);
        } else {
            ui->devices_list->addTopLevelItem(child);
        }
        align_columns();
        if (cummincation_device) {
            auto console = new QPlainTextEdit(ui->console_tabs);
            console->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
            console->setReadOnly(true);
            console->setMaximumBlockCount(1000);
            ui->console_tabs->addTab(console, tab_name);
            device_worker->connect_to_device_console(console, cummincation_device);
        }
    });
}

void MainWindow::add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *cummincation_device) {
    //called from device worker
    add_device_child_item(nullptr, item, tab_name, cummincation_device);
}

void MainWindow::append_html_to_console(QString text, QPlainTextEdit *console) {
    //is called from other threads
    Utility::thread_call(this, nullptr, [this, text, console] {
        assert(currently_in_gui_thread());
        if (console) {
            console->appendHtml(text);
        } else {
            Console::debug() << text;
        }
    });
}

void MainWindow::show_message_box(const QString &title, const QString &message, QMessageBox::Icon icon) {
    //is called from other threads
    Utility::thread_call(this, nullptr, [this, title, message, icon] {
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
    assert(currently_in_gui_thread());
    //  Utility::thread_call(this, nullptr, [this] {
    path_dialog = new PathSettingsWindow(this);
    path_dialog->show();
    //   / });
}

bool MainWindow::remove_device_item_recursion(QTreeWidgetItem *root_item, QTreeWidgetItem *child_to_remove, bool remove_if_existing) {
    int j = 0;
    while (j < root_item->childCount()) {
        QTreeWidgetItem *itemchild = root_item->child(j);
        if (itemchild == child_to_remove) {
            if (remove_if_existing) {
                root_item->removeChild(child_to_remove);
            }
            return true;
        } else {
            if (remove_device_item_recursion(itemchild, child_to_remove, remove_if_existing)) {
                return true;
            }
        }
        j++;
    }
    return false;
}

void MainWindow::remove_device_item(QTreeWidgetItem *child_to_remove) {
    assert(currently_in_gui_thread());
    remove_device_item_recursion(ui->devices_list->invisibleRootItem(), child_to_remove, true);
}

bool MainWindow::device_item_exists(QTreeWidgetItem *child) {
    assert(currently_in_gui_thread());
    return remove_device_item_recursion(ui->devices_list->invisibleRootItem(), child, false);
}

void MainWindow::get_devices_to_forget_by_root_treewidget_recursion(QList<QTreeWidgetItem *> &list, QTreeWidgetItem *root_item) {
    int j = 0;
    while (j < root_item->childCount()) {
        QTreeWidgetItem *itemchild = root_item->child(j);

        get_devices_to_forget_by_root_treewidget_recursion(list, itemchild);
        if (!list.contains(itemchild)) { //we want the children in the list first because of deletion order
            list.append(itemchild);
        }
        j++;
    }
}

QList<QTreeWidgetItem *> MainWindow::get_devices_to_forget_by_root_treewidget(QTreeWidgetItem *root_item) {
    assert(currently_in_gui_thread());
    QList<QTreeWidgetItem *> result;
    get_devices_to_forget_by_root_treewidget_recursion(result, root_item);
    return result;
}

void MainWindow::refresh_devices(bool only_duts) {
    assert(currently_in_gui_thread());

    statusBar()->showMessage(tr("Refreshing devices.."));
    ui->btn_refresh_all->setEnabled(false);
    ui->btn_refresh_dut->setEnabled(false);

    QTreeWidgetItem *root = ui->devices_list->invisibleRootItem();

    device_worker->refresh_devices(root, only_duts);
}

void MainWindow::slot_device_discovery_done() {
    ui->btn_refresh_all->setEnabled(true);
    ui->btn_refresh_dut->setEnabled(true);
    statusBar()->showMessage(tr(""));
}

void MainWindow::on_btn_refresh_all_clicked() {
    refresh_devices(false);
}

void MainWindow::on_btn_refresh_dut_clicked() {
    refresh_devices(true);
}

void MainWindow::on_run_test_script_button_clicked() {
    assert(currently_in_gui_thread());

    auto items = ui->tests_list->selectedItems();
    for (auto &item : items) {
        auto data = item->data(0, Qt::UserRole);
        auto test = Utility::from_qvariant<TestDescriptionLoader>(data);
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
        DeviceMatcher device_matcher(this);
        device_matcher.match_devices(*device_worker, runner, *test);
        auto devices = device_matcher.get_matched_devices();
        if (device_matcher.was_successful()) {
            runner.run_script(devices, *device_worker);
        } else {
            runner.interrupt();
        }
    }

}

void MainWindow::on_tests_list_itemClicked(QTreeWidgetItem *item, int column) {
    assert(currently_in_gui_thread());
    (void)column;
    //Utility::thread_call(this, nullptr, [this, item] {
    auto test = Utility::from_qvariant<TestDescriptionLoader>(item->data(0, Qt::UserRole));
    if (test == nullptr) {
        return;
    }
    Utility::replace_tab_widget(ui->test_tabs, 0, test->console.get(), test->get_name());
    ui->test_tabs->setCurrentIndex(0);
    // });
}

void MainWindow::on_tests_list_customContextMenuRequested(const QPoint &pos) {
    assert(currently_in_gui_thread());
    // Utility::thread_call(this, nullptr, [this, pos] {
    auto item = ui->tests_list->itemAt(pos);
    if (item && get_test_from_ui()) {
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
    //  });
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

void MainWindow::closeEvent(QCloseEvent *event) {
#if 1
    bool one_is_running = false;
    for (auto &test : test_runners) {
        if (test->is_running()) {
            one_is_running = true;
            break;
        }
    }
    if (one_is_running) {
        for (auto &test : test_runners) {
            if (test->is_running()) {
                test->pause_timers();
            }
        }

        if (QMessageBox::question(this, tr(""), tr("Scripts are still running. Abort them now?"), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
            for (auto &test : test_runners) {
                if (test->is_running()) {
                    test->resume_timers();
                    test->interrupt();
                    test->join();
                }
            }

        } else {
            event->ignore();
            for (auto &test : test_runners) {
                if (test->is_running()) {
                    test->resume_timers();
                }
            }
            return; //canceled closing the window
        }

        QApplication::processEvents();
        test_runners.clear();
        QApplication::processEvents();
    }
    event->accept();
#endif
}

void MainWindow::close_finished_tests() {
    auto &test_runners = MainWindow::mw->test_runners;
    test_runners.erase(std::remove_if(std::begin(test_runners), std::end(test_runners),
                                      [](const auto &test) {
                                          if (test->is_running()) {
                                              return false;
                                          }
                                          auto container = test->get_lua_ui_container();
                                          MainWindow::mw->ui->test_tabs->removeTab(MainWindow::mw->ui->test_tabs->indexOf(container));
                                          return true;
                                      }),
                       std::end(test_runners));
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
        runner.pause_timers();
        if (QMessageBox::question(this, tr(""), tr("Selected script %1 is still running. Abort it now?").arg(runner.get_name()),
                                  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
            runner.resume_timers();
            runner.interrupt();
            runner.join();
        } else {
            runner.resume_timers();
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
        connect(&action_close_finished, &QAction::triggered, &close_finished_tests);
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

void MainWindow::on_actionHotkey_triggered() {
    Hotkey_picker hp{this};
    hp.exec();
}

void MainWindow::poll_sg04_counts() {
#if 1
    assert(currently_in_gui_thread());
    QString sg04_prot_string = "SG04Count";

    auto sg04_count_devices = device_worker.get()->get_devices_with_protocol(sg04_prot_string, QStringList{""});
    for (auto &sg04_count_device : sg04_count_devices) {
        auto sg04_count_protocol = dynamic_cast<SG04CountProtocol *>(sg04_count_device->protocol.get());
        if (sg04_count_protocol) {
            unsigned int cps = sg04_count_protocol->get_actual_count_rate_cps();
            sg04_count_device->ui_entry->setText(2, "cps: " + QString::number(cps));
        }
    }

    QTimer::singleShot(500, this, &MainWindow::poll_sg04_counts);
#endif
}

void MainWindow::on_close_finished_tests_button_clicked() {
    close_finished_tests();
}

void MainWindow::on_actionDummy_Data_Creator_for_print_templates_triggered() {
    DummyDataCreator *dummydatacreator = new DummyDataCreator(this);
    dummydatacreator->show();
}
