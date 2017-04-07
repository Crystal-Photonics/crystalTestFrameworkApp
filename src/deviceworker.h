#ifndef DEVICEWORKER_H
#define DEVICEWORKER_H

#include "Protocols/protocol.h"
#include "qt_util.h"
#include "scriptengine.h"

#include <future>
#include <list>
#include <vector>
#include "scpimetadata.h"

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
    std::vector<ComportDescription *> get_devices_with_protocol(const QString &protocol, const QStringList device_names);
    void set_currently_running_test(CommunicationDevice *com_device, const QString &test_name);
	QStringList get_string_list(ScriptEngine &script, const QString &name);

	private:
    std::list<ComportDescription> comport_devices;
	void detect_devices(std::vector<ComportDescription *> comport_device_list);
    SCPIMetaData scpi_meta_data;
};

#endif // DEVICEWORKER_H
