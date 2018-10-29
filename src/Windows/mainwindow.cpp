#include "mainwindow.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "CommunicationDevices/usbtmccommunicationdevice.h"
#include "LuaUI/plot.h"
#include "LuaUI/window.h"
#include "Protocols/sg04countprotocol.h"
#include "Windows/dummydatacreator.h"
#include "Windows/infowindow.h"
#include "Windows/settingsform.h"
#include "config.h"
#include "console.h"
#include "devicematcher.h"
#include "deviceworker.h"
#include "identicon/identicon.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"
#include "testrunner.h"
#include "ui_container.h"
#include "ui_mainwindow.h"
#include "util.h"
#include <QAction>
#include <QByteArray>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
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

void MainWindow::load_default_paths_if_needed() {
    QString AppDataLocation = QStandardPaths::locate(QStandardPaths::AppDataLocation, QString{}, QStandardPaths::LocateDirectory);
    QMap<QString, QString> default_paths;
    default_paths.insert(Globals::test_script_path_settings_key, "examples/scripts/");
    default_paths.insert(Globals::isotope_source_data_base_path_key, "examples/settings/isotope_sources.json");
    default_paths.insert(Globals::device_protocols_file_settings_key, "examples/settings/communication_settings.json");
    default_paths.insert(Globals::measurement_equipment_meta_data_path_key, "examples/settings/equipment_data_base.json");
    default_paths.insert(Globals::path_to_environment_variables_key, "examples/settings/environment_variables.json");
    default_paths.insert(Globals::path_to_excpetional_approval_db_key, "examples/settings/exceptional_approvals.json");
    default_paths.insert(Globals::favorite_script_file_key, "examples/settings/favorite_scripts.json");
    default_paths.insert(Globals::rpc_xml_files_path_settings_key, "examples/xml/");

    QDir dir{AppDataLocation};

    {
        QStringList folders_for_copy{"scripts/", "xml/", "settings/"};
        QString source_base_dir = ":/examples/";
        QDirIterator it(source_base_dir, QDir::Files, QDirIterator::Subdirectories);
        QDir dir_source{source_base_dir};
        while (it.hasNext()) {
            bool accept_file = false;
            QString found_file = it.next();
            for (auto s : folders_for_copy) {
                if (found_file.startsWith(source_base_dir + s)) {
                    accept_file = true;
                    break;
                }
            }

            if (accept_file) {
                QString rel_path = dir_source.relativeFilePath(found_file);
                QString target_file_name = dir.absoluteFilePath("examples/" + rel_path);
                QString target_dir_s = QFileInfo{target_file_name}.absoluteDir().absolutePath();
                dir.mkpath(target_dir_s);
                if (!QFile::exists(target_file_name)) {
                    qDebug() << "copy example file: " << found_file << target_file_name;
                    QFile::copy(found_file, target_file_name);
                }
            }
        }
    }

    for (QString key : default_paths.keys()) {
        QString true_value = QSettings{}.value(key, "").toString();
        bool load_default_value = false;
        if (true_value.endsWith("/")) {
            load_default_value = !dir.exists(true_value);
        } else {
            load_default_value = !QFile::exists(true_value);
        }
        if (load_default_value) {
            QString default_value = default_paths[key];
            QString path = dir.absoluteFilePath(default_value);
            qDebug() << "set default setting: " << key << default_value << path;
            QSettings{}.setValue(key, path);
        }
    }
}

QString MainWindow::view_mode_to_string(MainWindow::ViewMode view_mode) {
    switch (view_mode) {
        case ViewMode::FavoriteScripts:
            return "favorite_only";
        case ViewMode::AllScripts:
            return "all_scripts";
        case ViewMode::None:
            return "";
    }
    return "";
}

MainWindow::ViewMode MainWindow::string_to_view_mode(QString view_mode_name) {
    if (view_mode_name == "favorite_only") {
        return ViewMode::FavoriteScripts;
    }
    if (view_mode_name == "all_scripts") {
        return ViewMode::AllScripts;
    }
    return ViewMode::None;
}

void MainWindow::set_view_mode(MainWindow::ViewMode view_mode) {
    switch (view_mode) {
        case ViewMode::None:
            break;
        case ViewMode::FavoriteScripts:
            enable_favorite_view();
            break;
        case ViewMode::AllScripts:
            enable_all_script_view();
            break;
    }
}

QStringList MainWindow::get_expanded_tree_view_recursion(QTreeWidgetItem *root_item, QString path) {
    QStringList result;
    path = path + root_item->text(0) + "/";
    result.append(path);
    for (int i = 0; i < root_item->childCount(); i++) {
        QTreeWidgetItem *item = root_item->child(i);
        if ((item->isExpanded()) && (item->childCount())) {
            result.append(get_expanded_tree_view_recursion(item, path));
        }
    }
    return result;
}

QStringList MainWindow::get_expanded_tree_view_paths() {
    QStringList result;
    for (int i = 0; i < ui->tests_advanced_view->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->tests_advanced_view->topLevelItem(i);
        if (item->isExpanded()) {
            result.append(get_expanded_tree_view_recursion(item, ""));
        }
    }
    return result;
}

void MainWindow::expand_from_stringlist_recusion(QTreeWidgetItem *root_item, const QStringList &child_texts, int index) {
    if (index > child_texts.count() - 1) {
        return;
    }
    QString child_text = child_texts[index];
    if (child_text == "") {
        return;
    }
    for (int i = 0; i < root_item->childCount(); i++) {
        QTreeWidgetItem *item = root_item->child(i);
        if (item->text(0) == child_text) {
            item->setExpanded(true);
            expand_from_stringlist_recusion(item, child_texts, index + 1);
        }
    }
}

void MainWindow::expand_from_stringlist(QStringList sl) {
    for (const auto &path : sl) {
        QStringList child_texts = path.split("/");
        QList<QTreeWidgetItem *> items = ui->tests_advanced_view->findItems(child_texts[0], Qt::MatchExactly, 0);
        for (QTreeWidgetItem *item : items) {
            item->setExpanded(true);
            expand_from_stringlist_recusion(item, child_texts, 1);
        }
    }
}

QString MainWindow::get_treeview_selection_path() {
    QString result;
    QTreeWidgetItem *item = ui->tests_advanced_view->currentItem();
    while (item != nullptr) {
        result = item->text(0) + "/" + result;
        item = item->parent();
    }
    return result;
}

QString MainWindow::get_list_selection_path() {
    QListWidgetItem *item = ui->test_simple_view->currentItem();
    if (item == nullptr) {
        return "";
    }
    TestDescriptionLoader *test = get_test_from_listViewItem(item);
    if (test == nullptr) {
        return "";
    }
    return test->get_name();
}

void MainWindow::set_list_selection_from_path(QString path) {
    ScriptEntry fav_entry = favorite_scripts.get_entry(path);
    QString text = fav_entry.alternative_name;
    if (text == "")
        text = fav_entry.script_path;

    QList<QListWidgetItem *> items = ui->test_simple_view->findItems(text, Qt::MatchExactly);
    for (auto item : items) {
        TestDescriptionLoader *test = get_test_from_listViewItem(item);
        if (test) {
            if (test->get_name() == path) {
                item->setSelected(true);
                ui->test_simple_view->setCurrentItem(item);
                return;
            }
        }
    }
}

void MainWindow::set_treeview_selection_from_path_recursion(QTreeWidgetItem *root_item, const QStringList &child_texts, int index) {
    if (index > child_texts.count() - 1) {
        return;
    }
    QString child_text = child_texts[index];
    if (child_text == "") {
        return;
    }
    for (int i = 0; i < root_item->childCount(); i++) {
        QTreeWidgetItem *item = root_item->child(i);
        if (item->text(0) == child_text) {
            item->setSelected(true);
            ui->tests_advanced_view->setCurrentItem(item);
        }
    }
}

void MainWindow::set_treeview_selection_from_path(QString path) {
    QStringList child_texts = path.split("/");
    if (child_texts.count() == 0) {
        return;
    }
    QList<QTreeWidgetItem *> items = ui->tests_advanced_view->findItems(child_texts[0], Qt::MatchExactly, 0);
    for (QTreeWidgetItem *item : items) {
        set_treeview_selection_from_path_recursion(item, child_texts, 1);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , favorite_scripts()
    , device_worker(std::make_unique<DeviceWorker>())
    , ui(new Ui::MainWindow) {
    MainWindow::gui_thread = QThread::currentThread();
    mw = this;
    ui->setupUi(this);

    //Utility::add_handle(ui->splitter_script_view);
    // Utility::add_handle(ui->splitter_3);

    load_default_paths_if_needed();
    favorite_scripts.load_from_file(QSettings{}.value(Globals::favorite_script_file_key, "").toString());
    device_worker->moveToThread(&devices_thread);
    QTimer::singleShot(500, this, &MainWindow::poll_sg04_counts);
    Console::console = ui->console_edit;
    Console::mw = this;
    // connect(&action_run, &QAction::triggered, [this] { on_run_test_script_button_clicked(); });

    ui->test_simple_view->setVisible(false);
    add_clear_button_to_console(ui->console_edit);
    devices_thread.start();
    connect(device_worker.get(), SIGNAL(device_discrovery_done()), this, SLOT(slot_device_discovery_done()));
    refresh_devices(false);

    load_scripts();
    ui->test_simple_view->installEventFilter(this);
    ViewMode vm = string_to_view_mode(QSettings{}.value(Globals::last_view_mode_key, "").toString());
    if (ui->test_simple_view->count() == 0) {
        vm = ViewMode::AllScripts;
    }
    if (vm == ViewMode::None) {
        vm = ViewMode::AllScripts;
    }
    set_view_mode(vm);
    set_console_view_collapse_state(QSettings{}.value(Globals::console_is_collapsed_key, true).toBool());
    expand_from_stringlist(QSettings{}.value(Globals::expanded_paths_key, QStringList{}).toStringList());
    set_treeview_selection_from_path(QSettings{}.value(Globals::current_tree_view_selection_key, "").toString());
    set_list_selection_from_path(QSettings{}.value(Globals::current_list_view_selection_key, "").toString());
    enable_run_test_button_by_script_selection();
    enable_closed_finished_test_button_script_states();
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::shutdown() {
	/*
	 * This function should be the destructor.
	 * However, according to http://eel.is/c++draft/basic.life#1.3 the lifetime of the MainWindow ends when it enters the destructor.
	 * We need MainWindow to stay alive until all other threads end, so we cannot end other thread in ~MainWindow, so we use shutdown instead.
	 * It is believed that using the destructor instead of shutdown has caused segfaults and race conditions in practice.
	 */
	assert(currently_in_gui_thread());
	QSettings{}.setValue(Globals::last_view_mode_key, view_mode_to_string(view_mode_m));
	QSettings{}.setValue(Globals::expanded_paths_key, get_expanded_tree_view_paths());
	QSettings{}.setValue(Globals::current_tree_view_selection_key, get_treeview_selection_path());
	QSettings{}.setValue(Globals::current_list_view_selection_key, get_list_selection_path());
	QSettings{}.setValue(Globals::console_is_collapsed_key, console_view_is_collapsed);
	for (auto &test : test_runners) {
		if (test->is_running()) {
			test->interrupt();
		}
	}

	auto done = std::async(std::launch::async, [this] {
		for (auto &test : test_runners) {
			test->join();
		}
		test_runners.clear();
		Utility::promised_thread_call(this, [&] {
			test_descriptions.clear(); //must clear descriptions in GUI thread because it touches GUI
		});
		devices_thread.quit();
		devices_thread.wait();
	});
	while (done.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
		QApplication::processEvents();
	}
	done.wait();

	QObject::disconnect(ui->tests_advanced_view, &QTreeWidget::itemSelectionChanged, this, &MainWindow::on_tests_advanced_view_itemSelectionChanged);
	QObject::disconnect(ui->test_simple_view, &QListWidget::itemSelectionChanged, this, &MainWindow::on_test_simple_view_itemSelectionChanged);
	QApplication::processEvents();
}

void MainWindow::align_columns() {
    assert(currently_in_gui_thread());
    int dev_min_size = 0;
    for (int i = 0; i < 4; i++) {
        ui->devices_list->resizeColumnToContents(i);
        dev_min_size += ui->devices_list->columnWidth(i);
    }
    dev_min_size -= dev_min_size % 50;
    dev_min_size += 50;
    ui->splitter_devices->setStretchFactor(0, 3);
    ui->splitter_devices->setStretchFactor(1, 1);
    ui->devices_list->setMinimumWidth(dev_min_size);
    for (int i = 0; i < ui->tests_advanced_view->columnCount(); i++) {
        ui->tests_advanced_view->resizeColumnToContents(i);
    }
}

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
    bool enabled_a = ui->tbtn_refresh_scripts->isEnabled();
    bool enabled_b = ui->actionReload_All_Scripts->isEnabled();
    ui->tbtn_refresh_scripts->setEnabled(false);
    ui->actionReload_All_Scripts->setEnabled(false);
    statusBar()->showMessage(tr("Refreshing Scripts.."));

    test_descriptions.clear();
    const auto dir = QSettings{}.value(Globals::test_script_path_settings_key, "").toString();
    QDirIterator dit{dir, QStringList{} << "*.lua", QDir::Files, QDirIterator::Subdirectories};
    while (dit.hasNext()) {
        const auto &file_path = dit.next();
        test_descriptions.push_back(TestDescriptionLoader{ui->tests_advanced_view, file_path, QDir{dir}.relativeFilePath(file_path)});
    }
    load_favorites();
    statusBar()->clearMessage();
    ui->tbtn_refresh_scripts->setEnabled(enabled_a);
    ui->actionReload_All_Scripts->setEnabled(enabled_b);
}

void MainWindow::load_favorites() {
    assert(currently_in_gui_thread());
    ui->test_simple_view->clear();
    QIcon icon_star = QIcon{"://src/icons/star_16.ico"};
    QIcon icon_empty_star = QIcon{"://src/icons/if_star_empty_16.png"};
    for (TestDescriptionLoader &test : test_descriptions) {
        const ScriptEntry favorite_entry = favorite_scripts.get_entry(test.get_name());
        if (favorite_entry.valid) {
            QListWidgetItem *item = new QListWidgetItem{favorite_entry.script_path};
            if (favorite_entry.alternative_name != "") {
                item->setText(favorite_entry.alternative_name);
            }
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            item->setData(Qt::UserRole, Utility::make_qvariant(test.ui_entry.get()));
            item->setToolTip(favorite_entry.script_path);
            Identicon ident_icon{test.get_name().toLocal8Bit()};
            const int SCALE_FACTOR = 12;
            QImage img = ident_icon.toImage(SCALE_FACTOR);
            item->setIcon(QPixmap::fromImage(img));
            ui->test_simple_view->addItem(item);
            test.ui_entry->setIcon(4, icon_star);
        } else {
            test.ui_entry->setIcon(4, icon_empty_star);
        }
    }
    set_enabled_states_for_matchable_scripts();
}

void MainWindow::set_enabled_states_for_matchable_scripts() {
    assert(currently_in_gui_thread());
    DeviceMatcher device_matcher(this);
    for (TestDescriptionLoader &test : test_descriptions) {
        bool is_matchable = device_matcher.is_match_possible(*device_worker.get(), test);
        if (is_matchable) {
            test.ui_entry->setForeground(0, palette().color(QPalette::Active, QPalette::Text));
            test.ui_entry->setForeground(1, palette().color(QPalette::Active, QPalette::Text));
            test.ui_entry->setForeground(3, palette().color(QPalette::Active, QPalette::Text));
        } else {
            test.ui_entry->setForeground(0, palette().color(QPalette::Disabled, QPalette::Text));
            test.ui_entry->setForeground(1, palette().color(QPalette::Disabled, QPalette::Text));
            test.ui_entry->setForeground(3, palette().color(QPalette::Disabled, QPalette::Text));
        }
    }
    for (int i = 0; i < ui->test_simple_view->count(); i++) {
        auto item = ui->test_simple_view->item(i);
        QTreeWidgetItem *tree_item = get_treewidgetitem_from_listViewItem(item);
        if (tree_item->foreground(0) == palette().color(QPalette::Active, QPalette::Text)) {
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        } else {
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable);
        }
    }
}

void MainWindow::add_device_child_item(QTreeWidgetItem *parent, QTreeWidgetItem *child, const QString &tab_name, CommunicationDevice *communication_device) {
    assert(currently_in_gui_thread() == false);
    Utility::promised_thread_call(this, [this, parent, child, tab_name, communication_device] {
        assert(currently_in_gui_thread());
        if (parent) {
            parent->addChild(child);
        } else {
            ui->devices_list->addTopLevelItem(child);
        }
        align_columns();
        if (communication_device) {
            int index = -1;
            for (int i = 0; i < ui->console_tabs->count(); i++) {
                if (ui->console_tabs->tabText(i) == tab_name) {
                    index = i;
                }
            }

            QPlainTextEdit *console = nullptr;
            if (index == -1) {
                console = new QPlainTextEdit(ui->console_tabs);
                console->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
                console->setReadOnly(true);
                console->setMaximumBlockCount(1000);
                ui->console_tabs->addTab(console, tab_name);
                add_clear_button_to_console(console);
            } else {
                console = dynamic_cast<QPlainTextEdit *>(ui->console_tabs->widget(index));
            }
            device_worker->connect_to_device_console(console, communication_device);
        }
    });
}

void MainWindow::add_clear_button_to_console(QPlainTextEdit *console) {
    QIcon icon_eraser = QIcon("://src/icons/if_eraser-small.ico");
    QToolButton *tbtn = new QToolButton(console);
    tbtn->setIcon(icon_eraser);
    tbtn->setToolTip(tr("Clear console content"));
    console->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    console->setCornerWidget(tbtn);
    connect(tbtn, &QToolButton::clicked, [console = console] { console->clear(); });
}

void MainWindow::set_testrunner_state(TestRunner *testrunner, TestRunner::State state) {
    assert(currently_in_gui_thread());
    QString prefix = " ";
    Qt::GlobalColor color = Qt::black;
    switch (state) {
        case TestRunner::State::running:
            prefix = "▶";
            color = Qt::darkGreen;
            break;
        case TestRunner::State::finished:
            prefix = "█";
            color = Qt::black;
            set_script_view_collapse_state(false);
            break;
        case TestRunner::State::error:
            prefix = "⚠";
            color = Qt::darkRed;
            set_script_view_collapse_state(false);
            break;
    }

    const auto runner_index = ui->test_tabs->indexOf(testrunner->get_lua_ui_container());
    const auto title = prefix + (" " + testrunner->get_name());
    if (runner_index == -1) {
        testrunner->get_lua_ui_container()->parentWidget()->setWindowTitle(title);
    } else {
        ui->test_tabs->setTabText(runner_index, title);
        ui->test_tabs->tabBar()->setTabTextColor(runner_index, color);
    }
    enable_closed_finished_test_button_script_states();
}

void MainWindow::adopt_testrunner(TestRunner *testrunner, QString title) {
    ui->test_tabs->addTab(testrunner->get_lua_ui_container(), title);
}

void MainWindow::show_status_bar_massage(QString msg, int timeout_ms) {
	Utility::thread_call(this, [this, msg, timeout_ms] {
        assert(currently_in_gui_thread());
        statusBar()->showMessage(msg, timeout_ms);
    });
}

void MainWindow::add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *communication_device) {
    //called from device worker
    assert(currently_in_gui_thread() == false);
    add_device_child_item(nullptr, item, tab_name, communication_device);
}

void MainWindow::append_html_to_console(QString text, QPlainTextEdit *console) {
    //might be called from other threads
	Utility::thread_call(this, [text, console] {
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
    assert(currently_in_gui_thread() == false);
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

void MainWindow::on_actionSettings_triggered() {
    assert(currently_in_gui_thread());
    path_dialog = new SettingsForm(this);
    path_dialog->show();
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
    ui->actionrefresh_devices_dut->setEnabled(false);
    ui->actionrefresh_devices_all->setEnabled(false);
    QTreeWidgetItem *root = ui->devices_list->invisibleRootItem();
    device_worker->refresh_devices(root, only_duts);
}

void MainWindow::slot_device_discovery_done() {
    ui->actionrefresh_devices_dut->setEnabled(true);
    ui->actionrefresh_devices_all->setEnabled(true);
    statusBar()->clearMessage();
    set_enabled_states_for_matchable_scripts();
}

void MainWindow::on_actionRunSelectedScript_triggered() {
    assert(currently_in_gui_thread());

    TestDescriptionLoader *test = nullptr;
    if (ui->tests_advanced_view->isVisible()) {
        auto item = ui->tests_advanced_view->selectedItems()[0];
        test = get_test_from_tree_widget(item);
    } else if (ui->test_simple_view->isVisible()) {
        auto item = ui->test_simple_view->selectedItems()[0];
        test = get_test_from_listViewItem(item);
    }
    if (test) {
        run_test_script(test);
    }
}

void MainWindow::on_actionedit_script_triggered() {
    assert(currently_in_gui_thread());

    TestDescriptionLoader *test = nullptr;
    if (ui->tests_advanced_view->isVisible()) {
        auto item = ui->tests_advanced_view->selectedItems()[0];
        test = get_test_from_tree_widget(item);
    } else if (ui->test_simple_view->isVisible()) {
        auto item = ui->test_simple_view->selectedItems()[0];
        test = get_test_from_listViewItem(item);
    }
    if (test) {
        test->launch_editor();
    }
}

void MainWindow::on_actionrefresh_devices_all_triggered() {
    refresh_devices(false);
}

void MainWindow::on_actionrefresh_devices_dut_triggered() {
    refresh_devices(true);
}

TestDescriptionLoader *MainWindow::get_test_from_tree_widget(const QTreeWidgetItem *item) {
    if (item == nullptr) {
        item = ui->tests_advanced_view->currentItem();
    }
    if (item->childCount() > 0) {
        item = nullptr;
    }
    if (item == nullptr) {
        return nullptr;
    }
    QVariant data = item->data(0, Qt::UserRole);
    return Utility::from_qvariant<TestDescriptionLoader>(data);
}

QTreeWidgetItem *MainWindow::get_treewidgetitem_from_listViewItem(QListWidgetItem *item) {
    return Utility::from_qvariant<QTreeWidgetItem>(item->data(Qt::UserRole));
}

TestDescriptionLoader *MainWindow::get_test_from_listViewItem(QListWidgetItem *item) {
    QTreeWidgetItem *parent_tree_widget_item = get_treewidgetitem_from_listViewItem(item);
    return get_test_from_tree_widget(parent_tree_widget_item);
}

void MainWindow::run_test_script(TestDescriptionLoader *test) {
    assert(currently_in_gui_thread());

    if (test == nullptr) {
        return;
    }
    try {
        test_runners.push_back(std::make_unique<TestRunner>(*test));
    } catch (const std::runtime_error &e) {
        Console::error(test->console) << "Failed running test: " << e.what();
        return;
    }
    auto &runner = *test_runners.back();
    const auto tab_index = ui->test_tabs->addTab(runner.get_lua_ui_container(), test->get_name());
    ui->test_tabs->setCurrentIndex(tab_index);
    DeviceMatcher device_matcher(this);
    device_matcher.match_devices(*device_worker, runner, *test);
    auto devices = device_matcher.get_matched_devices();
    if (device_matcher.was_successful()) {
        if (QSettings{}.value(Globals::collapse_script_explorer_on_scriptstart_key, false).toBool()) {
            set_script_view_collapse_state(true);
        }
        runner.run_script(devices, *device_worker);
    } else {
        runner.interrupt();
    }
}

void MainWindow::on_test_simple_view_itemDoubleClicked(QListWidgetItem *item) {
    (void)item;
    on_actionRunSelectedScript_triggered();
}

bool MainWindow::eventFilter(QObject *target, QEvent *event) {
    if (target == ui->test_simple_view) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Delete) {
                //qDebug() << "Qt::Key_Delete";
                remove_favorite_based_on_simple_view_selection();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(target, event);
}

void MainWindow::on_test_simple_view_itemSelectionChanged() {
    assert(currently_in_gui_thread());
    enable_run_test_button_by_script_selection();
}

void MainWindow::on_tests_advanced_view_itemClicked(QTreeWidgetItem *item, int column) {
    if (column == 4) {
        auto test = Utility::from_qvariant<TestDescriptionLoader>(item->data(0, Qt::UserRole));
        if (test) {
            QString test_name = test->get_name();
            bool modified = false;
            if (favorite_scripts.is_favorite(test_name)) {
                modified = favorite_scripts.remove_favorite(test_name);
            } else {
                modified = favorite_scripts.add_favorite(test_name);
            }
            if (modified) {
                load_favorites();
            }
            return;
        }
    }
}

void MainWindow::on_tests_advanced_view_itemSelectionChanged() {
    assert(currently_in_gui_thread());
    auto item = ui->tests_advanced_view->currentItem();
    if (item) {
        if (item->childCount() == 0) {
            auto test = Utility::from_qvariant<TestDescriptionLoader>(item->data(0, Qt::UserRole));
            if (test == nullptr) {
                return;
            }
            Utility::replace_tab_widget(ui->test_tabs, 0, test->console.get(), test->get_name());
            ui->test_tabs->setCurrentIndex(0);
        }
    }
    enable_run_test_button_by_script_selection();
}

void MainWindow::on_tests_advanced_view_itemDoubleClicked(QTreeWidgetItem *item, int column) {
    (void)item;
    (void)column;
    on_actionRunSelectedScript_triggered();
}

void MainWindow::on_tests_advanced_view_customContextMenuRequested(const QPoint &pos) {
    assert(currently_in_gui_thread());
    auto item = ui->tests_advanced_view->itemAt(pos);
    if (item && get_test_from_tree_widget()) {
        while (ui->tests_advanced_view->indexOfTopLevelItem(item) == -1) {
            item = item->parent();
        }
        emit on_tests_advanced_view_itemSelectionChanged();
        auto test = get_test_from_tree_widget();

        QMenu menu(this);

        QAction action_run(tr("Run"), nullptr);
        connect(&action_run, &QAction::triggered, [this] { on_actionRunSelectedScript_triggered(); });
        menu.addAction(&action_run);

        QAction action_reload(tr("Reload"), nullptr);
        connect(&action_reload, &QAction::triggered, [test] { test->reload(); });
        menu.addAction(&action_reload);

        QAction action_editor(tr("Open in Editor"), nullptr);
        connect(&action_editor, &QAction::triggered, [this] { on_actionedit_script_triggered(); });
        menu.addAction(&action_editor);

        QAction action_favorite(tr("Add to favorites"), nullptr);
        if (!favorite_scripts.is_favorite(test->get_name())) {
            connect(&action_favorite, &QAction::triggered, [test, this] {
                favorite_scripts.add_favorite(test->get_name());
                load_favorites();
                enable_favorite_view();
            });
            menu.addAction(&action_favorite);
        }
        menu.exec(ui->tests_advanced_view->mapToGlobal(pos));
    } else {
        QMenu menu(this);

        QAction action(tr("Reload all scripts"), nullptr);
        connect(&action, &QAction::triggered, [this] { on_tbtn_refresh_scripts_clicked(); });
        menu.addAction(&action);

        menu.exec(ui->tests_advanced_view->mapToGlobal(pos));
    }
}

void MainWindow::on_tbtn_refresh_scripts_clicked() {
    load_scripts();
}

void MainWindow::on_actionReload_All_Scripts_triggered() {
    on_tbtn_refresh_scripts_clicked();
}

void MainWindow::remove_favorite_based_on_simple_view_selection() {
    auto items = ui->test_simple_view->selectedItems();
    bool modified = false;
    for (auto item : items) {
        auto test = get_test_from_listViewItem(item);
        if (test) {
            if (favorite_scripts.remove_favorite(test->get_name())) {
                modified = true;
            }
        }
    }
    if (modified) {
        load_favorites();
    }
}

void MainWindow::on_test_simple_view_customContextMenuRequested(const QPoint &pos) {
    assert(currently_in_gui_thread());
    auto item = ui->test_simple_view->itemAt(pos);
    if (item == nullptr) {
        return;
    }
    QMenu menu(this);

    QAction action_run(tr("Run"), nullptr);
    connect(&action_run, &QAction::triggered, [this] { on_actionRunSelectedScript_triggered(); });
    menu.addAction(&action_run);

    QAction action_editor(tr("Open in Editor"), nullptr);
    connect(&action_editor, &QAction::triggered, [this] { on_actionedit_script_triggered(); });
    menu.addAction(&action_editor);

    QAction action_remove(tr("Remove from Favorites"), nullptr);
    connect(&action_remove, &QAction::triggered, [this] { remove_favorite_based_on_simple_view_selection(); });
    menu.addAction(&action_remove);

    menu.exec(ui->test_simple_view->mapToGlobal(pos));
}

void MainWindow::on_test_simple_view_itemChanged(QListWidgetItem *item) {
    auto test = get_test_from_listViewItem(item);
    favorite_scripts.set_alternative_name(test->get_name(), item->text());
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
    enable_closed_finished_test_button_script_states();
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
            connect(&action_abort_script, &QAction::triggered, [runner] {
                runner->interrupt();
                runner->join();
            });
            menu.addAction(&action_abort_script);
        }

        QAction action_open_script_in_editor(tr("Open in Editor"), nullptr);
        connect(&action_open_script_in_editor, &QAction::triggered, [runner] { runner->launch_editor(); });
        menu.addAction(&action_open_script_in_editor);

        QAction action_pop_out(tr("Open in extra Window"), nullptr);
        connect(&action_pop_out, &QAction::triggered, [this, runner] {
            auto container = runner->get_lua_ui_container();
            const auto index = ui->test_tabs->indexOf(container);
            new Window(runner, ui->test_tabs->tabBar()->tabText(index));
            ui->test_tabs->removeTab(index);
        });
        menu.addAction(&action_pop_out);

        menu.exec(ui->test_tabs->mapToGlobal(pos));
    } else {
        //clicked on the overview list
        QMenu menu(this);

        QAction action_close_finished(tr("Close finished Tests"), nullptr);
        //connect(&action_close_finished, &QAction::triggered, &close_finished_tests);
        connect(&action_close_finished, &QAction::triggered, [this] { close_finished_tests(); });
        menu.addAction(&action_close_finished);

        menu.exec(ui->test_tabs->mapToGlobal(pos));
    }
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

void MainWindow::on_actionDummy_Data_Creator_for_print_templates_triggered() {
    DummyDataCreator *dummydatacreator = new DummyDataCreator(this);
    if (dummydatacreator->get_is_valid_data_engine()) {
        dummydatacreator->show();
    }
}

void MainWindow::on_actionInfo_triggered() {
    auto *infowindow = new InfoWindow{this};
    infowindow->show();
}

void MainWindow::enable_favorite_view() {
    assert(currently_in_gui_thread());
    ui->tbtn_view_all_scripts->setChecked(false);
    ui->tbtn_view_favorite_scripts->setChecked(true);
    ui->test_simple_view->setVisible(true);
    ui->tests_advanced_view->setVisible(false);
    view_mode_m = ViewMode::FavoriteScripts;
    enable_run_test_button_by_script_selection();
}

void MainWindow::enable_all_script_view() {
    assert(currently_in_gui_thread());
    ui->tbtn_view_all_scripts->setChecked(true);
    ui->tbtn_view_favorite_scripts->setChecked(false);
    ui->test_simple_view->setVisible(false);
    ui->tests_advanced_view->setVisible(true);
    view_mode_m = ViewMode::AllScripts;
    enable_run_test_button_by_script_selection();
}

void MainWindow::enable_run_test_button_by_script_selection() {
    assert(currently_in_gui_thread());
    bool enabled = false;
    bool script_matchable = false;
    if (view_mode_m == ViewMode::AllScripts) {
        if (ui->tests_advanced_view->currentItem() == nullptr) {
            enabled = false;
        } else {
            if ((ui->tests_advanced_view->currentItem()->childCount() == 0)) {
                auto item = ui->tests_advanced_view->currentItem();
                TestDescriptionLoader *test = get_test_from_tree_widget(item);
                if (test) {
                    enabled = true;
                }
                if (item->foreground(0) == palette().color(QPalette::Active, QPalette::Text)) { //quite ugly
                    script_matchable = true;
                }
            }
        }
    } else {
        if (ui->test_simple_view->selectedItems().count() == 1) {
            auto item = ui->test_simple_view->currentItem();
            if (item) {
                if (item->flags() & Qt::ItemIsEnabled) {
                    script_matchable = true;
                }
                enabled = true;
            }
        } else {
            enabled = false;
        }
    }
    if (script_view_is_collapsed) {
        enabled = false;
    }
    ui->actionRunSelectedScript->setEnabled(enabled && script_matchable);
    ui->actionedit_script->setEnabled(enabled);
    if (enabled && script_matchable) {
        ui->actionRunSelectedScript->setToolTip(tr("Run selected Script"));
    } else {
        ui->actionRunSelectedScript->setToolTip(tr("Run(no script is selected)"));
    }
}

void MainWindow::enable_closed_finished_test_button_script_states() {
    assert(currently_in_gui_thread());
    bool enabled = false;
    for (const auto &tr : test_runners) {
        if (tr->is_running() == false) {
            enabled = true;
        }
    }
    ui->actionClose_finished_Tests->setEnabled(enabled);
    if (enabled) {
        ui->actionClose_finished_Tests->setToolTip(tr("Close finished script tabs"));
    } else {
        ui->actionClose_finished_Tests->setToolTip(tr("Close finished scripts(no script is finished)"));
    }
}

void MainWindow::set_script_view_collapse_state(bool collapse_state) {
    assert(currently_in_gui_thread());
    static QList<int> old_sizes;
    if (script_view_is_collapsed == false) {
        old_sizes = ui->splitter_script_view->sizes();
    }
    ui->frame_scripts->setVisible(!collapse_state);
    ui->frame_scripts_buttons->setVisible(!collapse_state);
    ui->frame_complete_script_panel->adjustSize();
    QIcon icon;
    if (collapse_state) {
        icon = QIcon{"://src/icons/if_bullet_arrow_down_5071.ico"};
        ui->tbtn_collapse_script_view->setIcon(icon);
        ui->splitter_script_view->setSizes(QList<int>{1, 50000});
    } else {
        icon = QIcon{"://src/icons/if_bullet_arrow_up_5073.ico"};
        ui->splitter_script_view->setSizes(old_sizes);
    }

    //  qDebug() << ui->splitter_script_view->sizes();
    ui->tbtn_collapse_script_view->setIcon(icon);
    script_view_is_collapsed = collapse_state;
    enable_run_test_button_by_script_selection();
}

void MainWindow::set_console_view_collapse_state(bool collapse_state) {
    assert(currently_in_gui_thread());
    static QList<int> old_sizes;
    if (console_view_is_collapsed == false) {
        old_sizes = ui->splitter_devices->sizes();
    }
    ui->console_tabs->setVisible(!collapse_state);
    QIcon icon;
    if (collapse_state) {
        icon = QIcon{"://src/icons/if_bullet_arrow_up_5073.ico"};
        ui->tbtn_collapse_script_view->setIcon(icon);
        ui->splitter_devices->setSizes(QList<int>{50000, 1});
    } else {
        icon = QIcon{"://src/icons/if_bullet_arrow_down_5071.ico"};
        ui->splitter_devices->setSizes(old_sizes);
    }

    ui->tbtn_collapse_console->setIcon(icon);
    console_view_is_collapsed = collapse_state;
}

void MainWindow::on_tbtn_collapse_console_clicked() {
    set_console_view_collapse_state(!console_view_is_collapsed);
}

void MainWindow::on_tbtn_collapse_script_view_clicked() {
    set_script_view_collapse_state(!script_view_is_collapsed);
}

void MainWindow::on_tbtn_view_all_scripts_clicked() {
    assert(currently_in_gui_thread());
    enable_all_script_view();
}

void MainWindow::on_tbtn_view_favorite_scripts_clicked() {
    assert(currently_in_gui_thread());
    enable_favorite_view();
}

void MainWindow::on_actionClose_finished_Tests_triggered() {
    assert(currently_in_gui_thread());
    close_finished_tests();
}
