#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "CommunicationDevices/comportcommunicationdevice.h"
#include "export.h"

#include <QMainWindow>
#include <QString>
#include <QtSerialPort/QSerialPortInfo>
#include <memory>
#include <set>
#include <vector>

class QTreeWidgetItem;

namespace Ui {
	class MainWindow;
}

class EXPORT MainWindow : public QMainWindow {
    Q_OBJECT

	public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	private slots:
	void on_actionPaths_triggered();
	void on_device_detect_button_clicked();
	void on_update_devices_list_button_clicked();
	void on_tabWidget_tabCloseRequested(int index);
	void poll_ports();

	private:
	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;

	struct ComportDescription{
		std::unique_ptr<ComportCommunicationDevice> device;
		QSerialPortInfo info;
		QTreeWidgetItem *ui_entry;
	};

	std::vector<ComportDescription> comport_devices;
};

#endif // MAINWINDOW_H
