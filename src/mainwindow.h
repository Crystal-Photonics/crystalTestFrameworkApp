#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/comportcommunicationdevice.h"
#include "Protocols/protocol.h"
#include "export.h"
#include "scriptengine.h"

#include <QMainWindow>
#include <QString>
#include <QTextEdit>
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

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

	struct ComportDescription;

	public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	public slots:
	void align_columns();

	private slots:
	void poll_ports();
	void forget_device();

	void on_actionPaths_triggered();
	void on_device_detect_button_clicked();
	void on_update_devices_list_button_clicked();
	void on_tabWidget_tabCloseRequested(int index);
	void on_run_test_script_button_clicked();
	void on_tests_list_itemClicked(QTreeWidgetItem *item, int column);
	void on_tests_list_customContextMenuRequested(const QPoint &pos);
	void on_devices_list_customContextMenuRequested(const QPoint &pos);

	private:
	void load_scripts();
	void detect_devices(std::vector<ComportDescription *> comport_device_list);

	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

	struct ComportDescription {
		std::unique_ptr<ComportCommunicationDevice> device;
		QSerialPortInfo info;
		QTreeWidgetItem *ui_entry;
		std::unique_ptr<Protocol> protocol;
	};

	std::vector<ComportDescription> comport_devices;

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
		QTextEdit *console = nullptr;
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
};

#endif // MAINWINDOW_H
