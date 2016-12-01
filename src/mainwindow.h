#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/comportcommunicationdevice.h"
#include "export.h"
#include "scriptengine.h"

#include <QMainWindow>
#include <QString>
#include <QtSerialPort/QSerialPortInfo>
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

	private:
	void load_scripts();

	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

	struct ComportDescription {
		std::unique_ptr<ComportCommunicationDevice> device;
		QSerialPortInfo info;
		QTreeWidgetItem *ui_entry;
	};

	std::vector<ComportDescription> comport_devices;

	struct Test {
		Test(QTreeWidget *w, const QString &file_path);
		~Test();
		Test(const Test &) = delete;
		Test(Test &&other);
		Test &operator=(const Test &) = delete;
		Test &operator=(Test &&other);
		void swap(Test &other);

		QTreeWidget *parent = nullptr;
		QTreeWidgetItem *ui_item = nullptr;
		ScriptEngine script;
		bool operator ==(QTreeWidgetItem *item){
			return item == ui_item;
		}
	};
	std::vector<Test> tests;
};

#endif // MAINWINDOW_H
