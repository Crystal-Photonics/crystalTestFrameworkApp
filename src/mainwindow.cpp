#include "mainwindow.h"
#include "config.h"
#include "pathsettingswindow.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QSettings>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <memory>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow) {
    ui->setupUi(this);
	ui->update_devices_list_button->click();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_actionPaths_triggered() {
	path_dialog = new PathSettingsWindow(this);
	path_dialog->show();
}

void MainWindow::on_device_detect_button_clicked() {}

void MainWindow::on_update_devices_list_button_clicked() {
	auto portlist = QSerialPortInfo::availablePorts();
	for (auto &port : portlist) {
		if (devices.count(port.systemLocation())) {
			continue;
		}
		devices.insert(CommunicationDevice::createConnection(port.systemLocation()));
		auto item = std::make_unique<QTreeWidgetItem>(ui->devices_list, QStringList{} << (QStringList{} << port.portName() << port.description()).join(" "));
		ui->devices_list->addTopLevelItem(item.release());
	}
}
