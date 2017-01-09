#include "worker.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "mainwindow.h"
#include "util.h"

#include <QDebug>
#include <QDirIterator>
#include <QMessageBox>
#include <QObject>
#include <QSettings>
#include <QTimer>

Worker::Worker(MainWindow *parent)
	: mw(parent) {
	emit poll_ports();
}

void Worker::poll_ports() {
	assert(currently_in_gui_thread() == false);
	for (auto &device : comport_devices) {
		if (device.device->isConnected()) {
			device.device->waitReceived(CommunicationDevice::Duration{0});
		}
	}
	QTimer::singleShot(16, this, &Worker::poll_ports);
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

void Worker::detect_devices(std::vector<ComportDescription *> comport_device_list) {
	auto device_protocol_settings_file = QSettings{}.value(Globals::device_protocols_file_settings_key, "").toString();
	if (device_protocol_settings_file.isEmpty()) {
		QMessageBox::critical(
			mw, tr("Missing File"),
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
				QMessageBox::critical(
					mw, tr("Input Error"),
					tr(R"(Invalid baudrate %1 specified in settings file "%2".)").arg(QString::number(baudrate), device_protocol_settings_file));
				continue;
			}
			if (device.device->connect(device.info, baudrate) == false) {
				Console::warning() << tr("Failed opening") << device.device->getTarget();
				return;
			}
			auto protocol = std::make_unique<RPCProtocol>(*device.device);
			if (protocol->is_correct_protocol()) {
				protocol->set_ui_description(device.ui_entry);
				device.protocol = std::move(protocol);

			} else {
				device.device->close();
			}
		}
		//TODO: Add non-rpc device discovery here
	};
	for (auto &device : comport_device_list) {
		for (auto &protocol_check_function : {check_rpc_protocols}) {
			if (device->device->isConnected()) {
				break;
			}
			protocol_check_function(*device);
		}
		if (device->device->isConnected() == false) { //out of protocols and still not connected
			Console::note() << tr("No protocol found for %1").arg(device->device->getTarget());
		}
	}
}

void Worker::forget_device(QTreeWidgetItem *item) {
	assert(currently_in_gui_thread() == false);

	for (auto device_it = std::begin(comport_devices); device_it != std::end(comport_devices); ++device_it) {
		if (device_it->ui_entry == item) {
			device_it = comport_devices.erase(device_it);

			break;
		}
	}
}

void Worker::update_devices() {
	assert(currently_in_gui_thread() == false);
	auto portlist = QSerialPortInfo::availablePorts();
	for (auto &port : portlist) {
		auto pos = std::lower_bound(std::begin(comport_devices), std::end(comport_devices), port.systemLocation(),
									[](const ComportDescription &lhs, const QString &rhs) { return lhs.device->getTarget() < rhs; });
		if (pos != std::end(comport_devices) && pos->device->getTarget() == port.systemLocation()) {
			continue;
		}
		auto item = std::make_unique<QTreeWidgetItem>(QStringList{} << port.portName() + " " + port.description());

		auto &device = *comport_devices.insert(pos, {std::make_unique<ComportCommunicationDevice>(port.systemLocation()), port, item.get(), nullptr})->device;
		emit mw->add_device_item(item.release(), port.portName() + " " + port.description(), &device);
	}
}

void Worker::detect_devices() {
	assert(currently_in_gui_thread() == false);
	std::vector<ComportDescription *> descriptions;
	descriptions.reserve(comport_devices.size());
	for (auto &comport : comport_devices) {
		descriptions.push_back(&comport);
	}
	detect_devices(descriptions);
}

void Worker::detect_device(QTreeWidgetItem *item) {
	assert(currently_in_gui_thread() == false);
	for (auto &comport : comport_devices) {
		if (comport.ui_entry == item) {
			detect_devices({&comport});
			break;
		}
	}
}

void Worker::connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport) {
	assert(currently_in_gui_thread() == false);
	struct Data {
		void (CommunicationDevice::*signal)(const QByteArray &);
		QColor color;
	};
	Data data[] = {{&CommunicationDevice::received, Qt::green},
				   {&CommunicationDevice::decoded_received, Qt::darkGreen},
				   {&CommunicationDevice::message, Qt::black},
				   {&CommunicationDevice::sent, Qt::red},
				   {&CommunicationDevice::decoded_sent, Qt::darkRed}};
	for (auto &d : data) {
		connect(comport, d.signal, [ console = console, color = d.color ](const QByteArray &data) {
			console->appendHtml("<font color=\"#" + QString::number(color.rgb(), 16) + "\"><plaintext>" + Utility::to_human_readable_binary_data(data) +
								"</plaintext></font>\n");
		});
	}
}

void Worker::get_devices_with_protocol(const QString &protocol, std::promise<std::vector<ComportDescription *>> &retval) {
	assert(currently_in_gui_thread() == false);
	std::vector<ComportDescription *> candidates;
	for (auto &device : comport_devices) { //TODO: do not only loop over comport_devices, but other devices as well
		if (device.protocol == nullptr) {
			continue;
		}
		if (device.protocol->type == protocol) {
			candidates.push_back(&device);
		}
	}
	retval.set_value(candidates);
}

void Worker::run_script(ScriptEngine *script, QPlainTextEdit *console, ComportDescription *device) {
	assert(currently_in_gui_thread() == false);
	try {
		Console::note(console) << "Started test";
		script->run({{*device->device, *device->protocol}});
	} catch (const sol::error &e) {
		Console::error(console) << e.what();
	}
}
