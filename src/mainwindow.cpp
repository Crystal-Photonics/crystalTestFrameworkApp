#include "mainwindow.h"
#include "Protocols/rpcprotocol.h"
#include "config.h"
#include "console.h"
#include "pathsettingswindow.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow) {
    ui->setupUi(this);
	Console::console = ui->console_edit;
	ui->update_devices_list_button->click();
	emit poll_ports();
	load_scripts();
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::align_device_columns() {
	for (int i = 0; i < ui->devices_list->columnCount(); i++) {
		ui->devices_list->resizeColumnToContents(i);
	}
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
			RPCProtocol protocol{*device.device};
			if (protocol.is_correct_protocol()) {
				protocol.set_ui_description(device.ui_entry);
				device.protocol = std::make_unique<RPCProtocol>(std::move(protocol));

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
		auto &device = *comport_devices.insert(pos, {std::make_unique<ComportCommunicationDevice>(port.systemLocation()), port, item.get(), nullptr})->device;
		ui->devices_list->addTopLevelItem(item.release());
		align_device_columns();

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

void MainWindow::load_scripts() {
	ui->tests_list->clear();
	QDirIterator dit{QSettings{}.value(Globals::test_script_path_settings_key, "").toString(), QStringList{} << "*.lua", QDir::Files};
	while (dit.hasNext()) {
		const auto &file_path = dit.next();
		tests.push_back({ui->tests_list, ui->test_tabs, file_path});
	}
}

MainWindow::Test::Test(QTreeWidget *test_list, QTabWidget *test_tabs, const QString &file_path)
	: parent(test_list)
	, test_tabs(test_tabs) {
	name = QString{file_path.data() + file_path.lastIndexOf('/') + 1};
	if (name.endsWith(".lua")) {
		name.chop(4);
	}
	console = new QTextEdit(test_tabs);
	test_tabs->addTab(console, name);

	ui_item = new QTreeWidgetItem(test_list, QStringList{} << name);
	ui_item->setData(0, Qt::UserRole, Utility::make_qvariant(this));
	parent->addTopLevelItem(ui_item);
	try {
		script.load_script(file_path);
	} catch (const sol::error &e) {
		Console::error(console) << tr("Failed loading script \"%1\" because %2").arg(file_path).arg(e.what());
		return;
	}
	try {
		QStringList protocols = script.get_string_list("protocols");
		std::copy(protocols.begin(), protocols.end(), std::back_inserter(this->protocols));
		std::sort(this->protocols.begin(), this->protocols.end());
		if (protocols.empty() == false) {
			ui_item->setText(1, protocols.join(", "));
		} else {
			ui_item->setText(1, tr("None"));
			ui_item->setTextColor(1, Qt::darkRed);
		}
	} catch (const sol::error &e) {
		Console::error(console) << tr("Failed retrieving variable \"protocols\" from %1 because %2").arg(file_path).arg(e.what());
		ui_item->setText(1, tr("Failed getting protocols"));
		ui_item->setTextColor(1, Qt::darkRed);
	}
}

MainWindow::Test::~Test() {
	if (ui_item != nullptr) {
		parent->removeItemWidget(ui_item, 0);
	}
}

MainWindow::Test::Test(MainWindow::Test &&other) {
	swap(other);
	ui_item->setData(0, Qt::UserRole, Utility::make_qvariant(this));
}

MainWindow::Test &MainWindow::Test::operator=(MainWindow::Test &&other) {
	swap(other);
	return *this;
}

void MainWindow::Test::swap(MainWindow::Test &other) {
	auto t = std::tie(this->parent, this->ui_item, this->script, this->protocols, this->name, this->test_tabs, this->console);
	auto o = std::tie(other.parent, other.ui_item, other.script, other.protocols, other.name, other.test_tabs, other.console);

	std::swap(t, o);
}

int MainWindow::Test::get_tab_id() const
{
	return test_tabs->indexOf(console);
}

void MainWindow::Test::activate_console()
{
	test_tabs->setCurrentIndex(get_tab_id());
}

void MainWindow::on_tests_list_itemDoubleClicked(QTreeWidgetItem *item, int column) {
	for (auto &test : tests) {
		if (test == item) {
			test.script.launch_editor();
		}
	}
}

void MainWindow::on_run_test_script_button_clicked() {
	auto items = ui->tests_list->selectedItems();
	for (auto &item : items) {
		auto test = Utility::from_qvariant<MainWindow::Test>(item->data(0, Qt::UserRole));
		if (test == nullptr) {
			continue;
		}
		if (test->protocols.empty()) {
			Console::error(test->console) << tr("The selected script \"%1\" cannot be run, because it did not report the required devices.").arg(test->name);
		}
		for (const auto &protocol : test->protocols) {
			std::vector<const ComportDescription *> candidates;
			for (const auto &device : comport_devices) { //TODO: do not only loop over comport_devices, but other devices as well
				if (device.protocol == nullptr) {
					continue;
				}
				if (device.protocol->type == protocol) {
					candidates.push_back(&device);
				}
			}
			switch (candidates.size()) {
				case 0:
					//failed to find suitable device
					Console::error(test->console) << tr("The selected test \"%1\" requires a device with protocol \"%2\", but no such device is available.")
														 .arg(test->ui_item->text(0))
														 .arg(protocol);
					break;
				case 1:
					//found the only viable option
					{
						auto &device = *candidates.front();
						auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol.get());
						if (rpc_protocol) { //we have an RPC protocol, so we have to ask the script if this RPC device is acceptable
							std::string message;
							try {
								message = test->script.call<std::string>("RPC_acceptable", rpc_protocol->get_lua_device_descriptor());
							} catch (const sol::error &e) {
								const auto &message = tr("Failed to call function RPC_acceptable.\nError message: %1").arg(e.what());
								Console::error(test->console) << message;
								return;
							}
							if (message.empty()) {
								//acceptable device
								QMessageBox::critical(this, "TODO", "TODO: implementation of accepted device");
							} else {
								//device incompatible, reason should be inside of message
								Console::note(test->console) << tr("Invalid device:") << message;
							}
						} else {
							assert(!"TODO: handle non-RPC protocol");
						}
					}
					break;
				default:
					//found multiple viable options
					QMessageBox::critical(this, "TODO", "TODO: implementation for multiple viable device options");
					break;
			}
		}
	}
}

void MainWindow::on_tests_list_itemClicked(QTreeWidgetItem *item, int column) {
	auto test = Utility::from_qvariant<MainWindow::Test>(item->data(0, Qt::UserRole));
	test->activate_console();
}
