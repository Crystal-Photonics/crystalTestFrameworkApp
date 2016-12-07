#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/comportcommunicationdevice.h"
#include "export.h"
#include "scriptengine.h"
#include "Protocols/protocol.h"

#include <QMainWindow>
#include <QString>
#include <QtSerialPort/QSerialPortInfo>
#include <memory>
#include <set>
#include <vector>
#include <list>
#include <QTextEdit>

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
	void align_device_columns();

	private slots:
	void on_actionPaths_triggered();
	void on_device_detect_button_clicked();
	void on_update_devices_list_button_clicked();
	void on_tabWidget_tabCloseRequested(int index);
	void poll_ports();

	void on_tests_list_itemDoubleClicked(QTreeWidgetItem *item, int column);

	void on_run_test_script_button_clicked();

	void on_tests_list_itemClicked(QTreeWidgetItem *item, int column);

	private:
	void load_scripts();

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
		ScriptEngine script;
		std::vector<QString> protocols;
		QString name;
		bool operator ==(QTreeWidgetItem *item){
			return item == ui_item;
		}
		int get_tab_id() const;
		void activate_console();
	};
	std::list<Test> tests;
};

#endif // MAINWINDOW_H
