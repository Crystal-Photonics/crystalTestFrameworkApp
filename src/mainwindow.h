#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/communicationdevice.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "Protocols/protocol.h"
#include "export.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "testdescriptionloader.h"
#include "testrunner.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QString>
#include <QThread>
#include <QtSerialPort/QSerialPortInfo>
#include <memory>
#include <vector>

class QTreeWidget;
class QTreeWidgetItem;
class DeviceWorker;

namespace Ui {
	class MainWindow;
}

namespace detail {
	extern QThread *gui_thread;
}

bool currently_in_gui_thread();

struct ComportDescription {
	std::unique_ptr<ComportCommunicationDevice> device;
	QSerialPortInfo info;
	QTreeWidgetItem *ui_entry;
	std::unique_ptr<Protocol> protocol;
};

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

	public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	static MainWindow *mw;

	template <class Function>
	void execute_in_gui_thread(Function &&f);

	void plot_create(int id, QSplitter *splitter);
	void plot_add_data(int id, double x, double y);
	void plot_add_data(int id, const std::vector<double> &data);
    void plot_add_data(int id, const unsigned int spectrum_start_channel, const std::vector<double> &data);
	void plot_clear(int id);
	void plot_drop(int id);
	void plot_set_offset(int id, double offset);
	void plot_set_gain(int id, double gain);

	void button_create(int id, QSplitter *splitter, const std::string &title, std::function<void()> callback);
	void button_drop(int id);
	void show_message_box(const QString &title, const QString &message, QMessageBox::Icon icon);

	public slots:
	void align_columns();
	void remove_device_entry(QTreeWidgetItem *item);
	void add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *comport);
	void append_html_to_console(QString text, QPlainTextEdit *console);

	private slots:
	void forget_device();
	void load_scripts();

	void on_actionPaths_triggered();
	void on_device_detect_button_clicked();
	void on_update_devices_list_button_clicked();
	void on_tabWidget_tabCloseRequested(int index);
	void on_run_test_script_button_clicked();
	void on_tests_list_itemClicked(QTreeWidgetItem *item, int column);
	void on_tests_list_customContextMenuRequested(const QPoint &pos);
	void on_devices_list_customContextMenuRequested(const QPoint &pos);
	void on_test_tabs_tabCloseRequested(int index);

	void on_test_tabs_customContextMenuRequested(const QPoint &pos);

	private:
	std::vector<TestDescriptionLoader> test_descriptions;
	std::vector<std::unique_ptr<TestRunner>> test_runners;

	std::unique_ptr<DeviceWorker> device_worker;
	QThread devices_thread;

	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

	TestDescriptionLoader *get_test_from_ui(const QTreeWidgetItem *item = nullptr);
	TestRunner *get_runner_from_tab_index(int index);
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
	Utility::thread_call(this, std::forward<Function>(f));
}

#endif // MAINWINDOW_H
