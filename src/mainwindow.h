#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "export.h"
#include "CommunicationDevices/communicationdevice.h"

#include <QMainWindow>
#include <QString>
#include <memory>
#include <vector>

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

	private:
	QDialog *path_dialog = nullptr;
    Ui::MainWindow *ui;
	std::vector<std::unique_ptr<CommunicationDevice>> devices;
};

#endif // MAINWINDOW_H
