#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "export.h"
#include "qt_util.h"
#include "testrunner.h"

#include "favorite_scripts.h"
#include <QDebug>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMessageBox>
#include <QThread>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPortInfo>
#include <map>
#include <memory>
#include <vector>

class CommunicationDevice;
class ComportCommunicationDevice;
class DeviceWorker;
class QPlainTextEdit;
class QSplitter;
class QString;
class QTreeWidget;
class TestDescriptionLoader;
class UI_container;
struct Protocol;

namespace Ui {
    class MainWindow;
}

bool currently_in_gui_thread();

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

    enum class ViewMode { None, AllScripts, FavoriteScripts };

    public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
	void shutdown();
    static MainWindow *mw;
    static QThread *gui_thread;

    template <class Function>
    void execute_in_gui_thread(Function f);

    template <class Lua_UI_class, class... Args>
    void add_lua_UI_class(int id, UI_container *parent, Args &&... args);
    template <class Lua_UI_class>
    void remove_lua_UI_class(int id);
    template <class Lua_UI_class>
    Lua_UI_class &get_lua_UI_class(int id);

    void show_message_box(const QString &title, const QString &message, QMessageBox::Icon icon);
    void remove_test_runner(TestRunner *runner);

    QList<QTreeWidgetItem *> get_devices_to_forget_by_root_treewidget(QTreeWidgetItem *root_item);
    void remove_device_item(QTreeWidgetItem *root_item);
    bool device_item_exists(QTreeWidgetItem *child);
    std::unique_ptr<QTreeWidgetItem> *get_manual_devices_parent_item();
    std::unique_ptr<QTreeWidgetItem> *create_manual_devices_parent_item();
    void add_device_child_item(QTreeWidgetItem *parent, QTreeWidgetItem *child, const QString &tab_name, CommunicationDevice *communication_device);
    void set_testrunner_state(TestRunner *testrunner, TestRunner::State state);
    void adopt_testrunner(TestRunner *testrunner, QString title);
    void show_status_bar_massage(QString msg, int timeout_ms);

    public slots:
    void align_columns();

    void add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *communication_device);
    void append_html_to_console(QString text, QPlainTextEdit *console);

    private slots:
    void slot_device_discovery_done();
    void load_scripts();

    void on_actionSettings_triggered();
    void on_tests_advanced_view_customContextMenuRequested(const QPoint &pos);
    void on_test_tabs_tabCloseRequested(int index);
    void on_test_tabs_customContextMenuRequested(const QPoint &pos);
    void on_console_tabs_customContextMenuRequested(const QPoint &pos);
    void on_actionDummy_Data_Creator_for_print_templates_triggered();
    void on_actionInfo_triggered();

	void poll_sg04_counts();
    void closeEvent(QCloseEvent *event) override;

    void on_test_simple_view_itemDoubleClicked(QListWidgetItem *item);
    void on_test_simple_view_customContextMenuRequested(const QPoint &pos);
    void on_test_simple_view_itemChanged(QListWidgetItem *item);
    void on_tbtn_view_all_scripts_clicked();
    void on_tbtn_view_favorite_scripts_clicked();
    void on_actionRunSelectedScript_triggered();
    void on_actionrefresh_devices_all_triggered();
    void on_actionrefresh_devices_dut_triggered();
    void on_actionClose_finished_Tests_triggered();
    void on_test_simple_view_itemSelectionChanged();
    void on_tests_advanced_view_itemSelectionChanged();
    void on_tests_advanced_view_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_tbtn_collapse_script_view_clicked();
    void on_tests_advanced_view_itemClicked(QTreeWidgetItem *item, int column);
    void on_actionedit_script_triggered();
    void on_tbtn_refresh_scripts_clicked();
    void on_actionReload_All_Scripts_triggered();
    void on_tbtn_collapse_console_clicked();

    private:
    FavoriteScripts favorite_scripts;
    void refresh_devices(bool only_duts);
    std::vector<TestDescriptionLoader> test_descriptions;
    std::vector<std::unique_ptr<TestRunner>> test_runners;

    std::unique_ptr<DeviceWorker> device_worker;
    QThread devices_thread;

    QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

    // TestDescriptionLoader *get_test_from_ui(const QTreeWidgetItem *item = nullptr);
    TestRunner *get_runner_from_tab_index(int index);

    void close_finished_tests();
    void get_devices_to_forget_by_root_treewidget_recursion(QList<QTreeWidgetItem *> &list, QTreeWidgetItem *root_item);
    bool remove_device_item_recursion(QTreeWidgetItem *root_item, QTreeWidgetItem *child_to_remove, bool remove_if_existing);
    std::unique_ptr<QTreeWidgetItem> manual_devices_parent_item;
    QMutex manual_devices_parent_item_mutex;
    void load_default_paths_if_needed();
    void run_test_script(TestDescriptionLoader *test);
    TestDescriptionLoader *get_test_from_listViewItem(QListWidgetItem *item);
    TestDescriptionLoader *get_test_from_tree_widget(const QTreeWidgetItem *item = nullptr);
    void load_favorites();
    void enable_favorite_view();
    void enable_all_script_view();
    ViewMode view_mode_m;
    QString view_mode_to_string(ViewMode view_mode);
    ViewMode string_to_view_mode(QString view_mode_name);
    void set_view_mode(ViewMode view_mode);

    QStringList get_expanded_tree_view_paths();

    QStringList get_expanded_tree_view_recursion(QTreeWidgetItem *root_item, QString path);
    void expand_from_stringlist(QStringList sl);
    void expand_from_stringlist_recusion(QTreeWidgetItem *root_item, const QStringList &child_texts, int index);
    QString get_treeview_selection_path();
    void set_treeview_selection_from_path(QString path);
    void set_treeview_selection_from_path_recursion(QTreeWidgetItem *root_item, const QStringList &child_texts, int index);
    QString get_list_selection_path();
    void set_list_selection_from_path(QString path);
    void enable_run_test_button_by_script_selection();
    void enable_closed_finished_test_button_script_states();
    void set_script_view_collapse_state(bool collapse_state);
    bool script_view_is_collapsed = false;
    void set_console_view_collapse_state(bool collapse_state);
    bool console_view_is_collapsed = false;
    bool eventFilter(QObject *target, QEvent *event);

    void remove_favorite_based_on_simple_view_selection();
    void set_enabled_states_for_matchable_scripts();
    QTreeWidgetItem *get_treewidgetitem_from_listViewItem(QListWidgetItem *item);
    void add_clear_button_to_console(QPlainTextEdit *console);
};

template <class T>
bool operator==(const std::unique_ptr<T> &lhs, const T *rhs) {
    return lhs.get() == rhs;
}

template <class T>
bool operator==(const T *lhs, const std::unique_ptr<T> &rhs) {
    return lhs == rhs.get();
}

template <class Function>
void MainWindow::execute_in_gui_thread(Function f) {
	Utility::thread_call(this, std::move(f));
}

template <class Lua_UI_class>
std::map<int, Lua_UI_class> lua_classes;

template <class Lua_UI_class, class... Args>
void MainWindow::add_lua_UI_class(int id, UI_container *parent, Args &&... args) {
    lua_classes<Lua_UI_class>.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(parent, std::forward<Args>(args)...));
}

template <class Lua_UI_class>
void MainWindow::remove_lua_UI_class(int id) {
    lua_classes<Lua_UI_class>.erase(id);
}

template <class Lua_UI_class>
Lua_UI_class &MainWindow::get_lua_UI_class(int id) {
    return lua_classes<Lua_UI_class>.at(id);
}

#endif // MAINWINDOW_H
