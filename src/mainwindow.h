#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/communicationdevice.h"
#include "Protocols/protocol.h"
#include "export.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "worker.h"

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QString>
#include <QThread>
#include <QtSerialPort/QSerialPortInfo>
#include <list>
#include <memory>
#include <set>
#include <vector>

class QTreeWidget;
class QTreeWidgetItem;

namespace Ui {
	class MainWindow;
}

namespace detail {
	extern QThread *gui_thread;
}

bool currently_in_gui_thread();

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

	public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	template <class Function>
	void execute_in_gui_thread(Function &&f);

	public slots:
	void align_columns();
	void remove_device_entry(QTreeWidgetItem *item);
	void add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *comport);
	void append_html_to_console(QString text, QPlainTextEdit *console);
	void create_plot(int id, QSplitter *splitter);
	void add_data_to_plot(int id, double x, double y);
	void clear_plot(int id);

	private slots:
	void forget_device();
	void debug_channel_codec_state(std::list<DeviceProtocol> &protocols);
	void load_scripts();

	void on_actionPaths_triggered();
	void on_device_detect_button_clicked();
	void on_update_devices_list_button_clicked();
	void on_tabWidget_tabCloseRequested(int index);
	void on_run_test_script_button_clicked();
	void on_tests_list_itemClicked(QTreeWidgetItem *item, int column);
	void on_tests_list_customContextMenuRequested(const QPoint &pos);
	void on_devices_list_customContextMenuRequested(const QPoint &pos);

	private:
	struct Test {
		Test(QTreeWidget *test_list, QTabWidget *test_tabs, const QString &file_path);
		~Test();
		Test(const Test &) = delete;
		Test(Test &&other);
		Test &operator=(const Test &) = delete;
		Test &operator=(Test &&other);
		void swap(Test &other);

		QTreeWidget *parent = nullptr;
		QTreeWidgetItem *ui_item = nullptr;
		QTabWidget *test_tabs = nullptr;
		QPlainTextEdit *console = nullptr;
		QSplitter *splitter = nullptr;
		ScriptEngine script;
		std::vector<QString> protocols;
		QString name;
		QString file_path;
		bool operator==(QTreeWidgetItem *item);
		int get_tab_id() const;
		void activate_console();
	};
	std::list<Test> tests;

	std::unique_ptr<Worker> worker;
	QThread worker_thread;

	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

	Test *get_test_from_ui();
};

template <class Function>
void MainWindow::execute_in_gui_thread(Function &&f) {
	Utility::thread_call(this, std::forward<Function>(f));
}

#endif // MAINWINDOW_H
