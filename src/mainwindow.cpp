#include "mainwindow.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "pathsettingswindow.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"
#include "console.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <algorithm>
#include <memory>

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow) {
    ui->setupUi(this);
	Console::console = ui->console_edit;
	ui->update_devices_list_button->click();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_actionPaths_triggered() {
	path_dialog = new PathSettingsWindow(this);
	path_dialog->show();
}

static bool is_valid_baudrate(int baudrate) {
	switch (baudrate) {
		case QSerialPort::Baud9600:
		case QSerialPort::Baud57600:
		case QSerialPort::Baud4800:
		case QSerialPort::Baud38400:
		case QSerialPort::Baud2400:
		case QSerialPort::Baud19200:
		case QSerialPort::Baud115200:
			return true;
	}
	return false;
}

void MainWindow::on_device_detect_button_clicked() {
	auto device_protocol_settings_file = QSettings{}.value(Globals::device_protocols_file_settings_key, "").toString();
	if (device_protocol_settings_file.isEmpty()) {
		QMessageBox::critical(
			this, tr("Missing File"),
			tr("Auto-Detecting devices requires a file that defines which protocols can use which file. Make such a file and add it via Settings->Paths"));
		return;
	}
	QSettings device_protocol_settings{device_protocol_settings_file, QSettings::IniFormat};
	auto rpc_devices = device_protocol_settings.value("RPC").toStringList();
	for (auto &rpc_device : rpc_devices) {
		if (rpc_device.startsWith("COM:")) {
			for (auto &device : comport_devices) {
				if (device->isConnected()) {
					continue;
				}

				const auto baudrate = rpc_device.split(":")[1];
				if (is_valid_baudrate(baudrate.toInt()) == false) {
					QMessageBox::critical(this, tr("Input Error"),
										  tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(baudrate).arg(device_protocol_settings_file));
					continue;
				}
				if (device->waitConnected(100ms, baudrate) == false){
					Console::warning() << "Failed opening" << device->getTarget();
					continue;
				}
				if (RPCProtocol::is_correct_protocol(*device)) {
					//TODO
				}
			}
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
