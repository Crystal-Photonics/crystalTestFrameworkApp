#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/communicationdevice.h"
#include "Protocols/protocol.h"
#include "export.h"
#include "scriptengine.h"
#include "worker.h"

#include <QMainWindow>
#include <QString>
#include <QPlainTextEdit>
#include <QtSerialPort/QSerialPortInfo>
#include <list>
#include <memory>
#include <set>
#include <vector>
#include <QThread>

class QTreeWidget;
class QTreeWidgetItem;

namespace Ui {
	class MainWindow;
}

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

	public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	public slots:
	void align_columns();
	void remove_device_entry(QTreeWidgetItem *item);
	void add_device_item(QTreeWidgetItem *item, const QString &tab_name, CommunicationDevice *comport);
	void append_html_to_console(QString text, QPlainTextEdit *console);

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

#endif // MAINWINDOW_H
