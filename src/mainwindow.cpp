#include "mainwindow.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "pathsettingswindow.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <algorithm>
#include <cassert>
#include <memory>

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow) {
    ui->setupUi(this);
	Console::console = ui->console_edit;
	ui->update_devices_list_button->click();
	emit poll_ports();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_actionPaths_triggered() {
	path_dialog = new PathSettingsWindow(this);
	path_dialog->show();
}

static bool is_valid_baudrate(QSerialPort::BaudRate baudrate) {
	switch (baudrate) {
		case QSerialPort::Baud1200:
		case QSerialPort::Baud2400:
		case QSerialPort::Baud4800:
		case QSerialPort::Baud9600:
		case QSerialPort::Baud38400:
		case QSerialPort::Baud57600:
		case QSerialPort::Baud19200:
		case QSerialPort::Baud115200:
			return true;
		case QSerialPort::UnknownBaud:
			return false;
	}
	return false;
}

void MainWindow::on_device_detect_button_clicked() {
	Console::note() << "Auto-detecting protocols for devices";
	auto device_protocol_settings_file = QSettings{}.value(Globals::device_protocols_file_settings_key, "").toString();
	if (device_protocol_settings_file.isEmpty()) {
		QMessageBox::critical(
			this, tr("Missing File"),
			tr("Auto-Detecting devices requires a file that defines which protocols can use which file. Make such a file and add it via Settings->Paths"));
		return;
	}
	QSettings device_protocol_settings{device_protocol_settings_file, QSettings::IniFormat};
	auto rpc_devices = device_protocol_settings.value("RPC").toStringList();
	auto check_rpc_protocols = [&](auto &device) {
		for (auto &rpc_device : rpc_devices) {
			if (rpc_device.startsWith("COM:") == false) {
				continue;
			}
			const QSerialPort::BaudRate baudrate = static_cast<QSerialPort::BaudRate>(rpc_device.split(":")[1].toInt());
			if (is_valid_baudrate(baudrate) == false) {
				QMessageBox::critical(this, tr("Input Error"),
									  tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(baudrate).arg(device_protocol_settings_file));
				continue;
			}
			if (device.device->connect(device.info, baudrate) == false) {
				Console::warning() << tr("Failed opening") << device.device->getTarget();
				return;
			}
			RPCProtocol protocol;
			if (protocol.is_correct_protocol(*device.device)) {
				protocol.set_ui_description(*device.device, device.ui_entry);
			} else {
				device.device->close();
			}
		}
		//TODO: Add non-rpc device discovery here
	};
	for (auto &device : comport_devices) {
		for (auto &protocol_check_function : {check_rpc_protocols}) {
			if (device.device->isConnected()) {
				break;
			}
			protocol_check_function(device);
		}
		if (device.device->isConnected() == false) { //out of protocols and still not connected
			Console::note() << tr("No protocol found for %1").arg(device.device->getTarget());
		}
	}
}

void MainWindow::on_update_devices_list_button_clicked() {
	auto portlist = QSerialPortInfo::availablePorts();
	for (auto &port : portlist) {
		auto pos = std::lower_bound(std::begin(comport_devices), std::end(comport_devices), port.systemLocation(),
									[](const MainWindow::ComportDescription &lhs, const QString &rhs) { return lhs.device->getTarget() < rhs; });
		if (pos != std::end(comport_devices) && pos->device->getTarget() == port.systemLocation()) {
			continue;
		}
		auto item = std::make_unique<QTreeWidgetItem>(ui->devices_list, QStringList{} << port.portName() + " " + port.description());
		auto &device = *comport_devices.insert(pos, {std::make_unique<ComportCommunicationDevice>(port.systemLocation()), port, item.get()})->device;
		ui->devices_list->addTopLevelItem(item.release());

		auto tab = new QTextEdit(ui->tabWidget);
		tab->setReadOnly(true);
		ui->tabWidget->addTab(tab, port.portName() + " " + port.description());
		static const auto percent_encoding_include = " :\t\\\n!\"§$%&/()=+-*";

		struct Data {
			void (CommunicationDevice::*signal)(const QByteArray &);
			QColor color;
			QFont::Weight weight;
		};
		Data data[] = {{&CommunicationDevice::received, Qt::darkGreen, QFont::Light},
					   {&CommunicationDevice::decoded_received, Qt::darkGreen, QFont::DemiBold},
					   {&CommunicationDevice::message, Qt::black, QFont::DemiBold},
					   {&CommunicationDevice::sent, Qt::darkRed, QFont::Light},
					   {&CommunicationDevice::decoded_sent, Qt::darkRed, QFont::DemiBold}};
		for (auto &d : data) {
			connect(&device, d.signal, [ console = tab, color = d.color, weight = d.weight ](const QByteArray &data) {
				console->setTextColor(color);
				console->setFontWeight(weight);
				console->append(data.toPercentEncoding(percent_encoding_include));
			});
		}
	}
}

void MainWindow::on_tabWidget_tabCloseRequested(int index) {
	if (ui->tabWidget->tabText(index) == "Console") {
		Console::note() << tr("Cannot close console window");
		return;
	}
	ui->tabWidget->removeTab(index);
}

void MainWindow::poll_ports() {
	for (auto &device : comport_devices) {
		if (device.device->isConnected()) {
			device.device->waitReceived(CommunicationDevice::Duration{0});
		}
	}
	QTimer::singleShot(16, this, &MainWindow::poll_ports);
}
