#ifndef WORKER_H
#define WORKER_H

#include "CommunicationDevices/comportcommunicationdevice.h"
#include "Protocols/protocol.h"
#include "scriptengine.h"

#include <QPlainTextEdit>
#include <QSplitter>
#include <QTreeWidgetItem>
#include <QtSerialPort/QSerialPort>
#include <functional>
#include <future>
#include <list>
#include <memory>

class MainWindow;

struct ComportDescription {
	std::unique_ptr<ComportCommunicationDevice> device;
	QSerialPortInfo info;
	QTreeWidgetItem *ui_entry;
	std::unique_ptr<Protocol> protocol;
};

class Worker : public QObject {
	Q_OBJECT
	public:
	Worker(MainWindow *parent);

	std::list<ComportDescription> comport_devices;

	public slots:
	void forget_device(QTreeWidgetItem *item);
	void update_devices();
	void detect_devices();
	void detect_device(QTreeWidgetItem *item);
	void connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport);
	void get_devices_with_protocol(const QString &protocol, std::promise<std::vector<ComportDescription *>> &retval);
	void run_script(ScriptEngine *script, QPlainTextEdit *console, ComportDescription *device);
	ScriptEngine::State get_state(ScriptEngine &script);
	void abort_script(ScriptEngine &script);

	private slots:
	void poll_ports();

	private:
	MainWindow *mw;
	void detect_devices(std::vector<ComportDescription *> comport_device_list);
};

#endif // WORKER_H
