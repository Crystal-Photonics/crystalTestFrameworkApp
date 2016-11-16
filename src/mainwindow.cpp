#include "mainwindow.h"
#include "Protocols/rpcprotocol.h"
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
#include <algorithm>
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

void MainWindow::on_device_detect_button_clicked() {
	for (auto &device : comport_devices) {
		if (RPCProtocol::is_correct_protocol(*device)) {
			//TODO
		}
	}
}

void MainWindow::on_update_devices_list_button_clicked() {
	auto portlist = QSerialPortInfo::availablePorts();
	for (auto &port : portlist) {
		auto pos = std::lower_bound(std::begin(comport_devices), std::end(comport_devices), port.systemLocation(),
									[](const std::unique_ptr<ComportCommunicationDevice> &lhs, const QString &rhs) { return lhs->getTarget() < rhs; });
		if (pos != std::end(comport_devices) && (*pos)->getTarget() == port.systemLocation()) {
			continue;
		}
		comport_devices.insert(pos, std::make_unique<ComportCommunicationDevice>(port.systemLocation()));
		auto item = std::make_unique<QTreeWidgetItem>(ui->devices_list, QStringList{} << (QStringList{} << port.portName() << port.description()).join(" "));
		ui->devices_list->addTopLevelItem(item.release());
	}
}
