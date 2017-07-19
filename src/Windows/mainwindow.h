#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "export.h"
#include "qt_util.h"

#include <QDebug>
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
class TestRunner;
class UI_container;
struct Protocol;

namespace Ui {
    class MainWindow;
}

bool currently_in_gui_thread();

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

    public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    static MainWindow *mw;
    static QThread *gui_thread;

    template <class Function>
    void execute_in_gui_thread(Function &&f);

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
    void add_device_child_item(QTreeWidgetItem *parent, QTreeWidgetItem *child, const QString &tab_name, CommunicationDevice *cummincation_device);
public slots:
    void align_columns();

    void add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *cummincation_device);
    void append_html_to_console(QString text, QPlainTextEdit *console);

    private slots:
    void slot_device_discovery_done();
    void load_scripts();

    void on_actionPaths_triggered();

    void on_run_test_script_button_clicked();
    void on_tests_list_itemClicked(QTreeWidgetItem *item, int column);
    void on_tests_list_customContextMenuRequested(const QPoint &pos);
    void on_test_tabs_tabCloseRequested(int index);

    void on_test_tabs_customContextMenuRequested(const QPoint &pos);

    void on_use_human_readable_encoding_toggled(bool checked);

    void on_console_tabs_customContextMenuRequested(const QPoint &pos);

    void on_actionHotkey_triggered();

    void poll_sg04_counts();
    void on_close_finished_tests_button_clicked();

    void on_actionDummy_Data_Creator_for_print_templates_triggered();
    void on_btn_refresh_dut_clicked();
    void on_btn_refresh_all_clicked();

    void closeEvent(QCloseEvent *event) override;

    void on_actionInfo_triggered();

private:
    void refresh_devices(bool only_duts);
    std::vector<TestDescriptionLoader> test_descriptions;
    std::vector<std::unique_ptr<TestRunner>> test_runners;

    std::unique_ptr<DeviceWorker> device_worker;
    QThread devices_thread;

    QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

    TestDescriptionLoader *get_test_from_ui(const QTreeWidgetItem *item = nullptr);
    TestRunner *get_runner_from_tab_index(int index);

    static void close_finished_tests();
    void get_devices_to_forget_by_root_treewidget_recursion(QList<QTreeWidgetItem *> &list, QTreeWidgetItem *root_item);
    bool remove_device_item_recursion(QTreeWidgetItem *root_item, QTreeWidgetItem *child_to_remove, bool remove_if_existing);
    std::unique_ptr<QTreeWidgetItem> manual_devices_parent_item;
    QMutex manual_devices_parent_item_mutex;
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
void MainWindow::execute_in_gui_thread(Function &&f) {
    Utility::thread_call(this, nullptr, std::forward<Function>(f));
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
