#ifndef DEVICEWORKER_H
#define DEVICEWORKER_H

#include "Protocols/protocol.h"
#include "qt_util.h"
#include "scriptengine.h"

#include <future>
#include <list>
#include <vector>

class CommunicationDevice;
class MainWindow;
class QPlainTextEdit;
class QTreeWidgetItem;
struct ComportDescription;

class DeviceWorker : public QObject {
	Q_OBJECT
	public:
	~DeviceWorker();
	void forget_device(QTreeWidgetItem *item);
	void update_devices();
	void detect_devices();
	void detect_device(QTreeWidgetItem *item);
	void connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport);
	void get_devices_with_protocol(const QString &protocol, std::promise<std::vector<ComportDescription *>> &retval);
	void set_currently_running_test(CommunicationDevice *com_device, const QString &test_name) const;
	QStringList get_string_list(ScriptEngine &script, const QString &name);

	private slots:
	void poll_ports();

	private:
	std::list<ComportDescription> comport_devices;
	void detect_devices(std::vector<ComportDescription *> comport_device_list);
};

#endif // DEVICEWORKER_H
