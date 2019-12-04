#ifndef DEVICEWORKER_H
#define DEVICEWORKER_H

#include "CommunicationDevices/libusbscan.h"
#include "CommunicationDevices/usbtmc.h"
#include "Protocols/protocol.h"
#include "qt_util.h"
#include "scpimetadata.h"
#include "scriptengine.h"

#include <QSemaphore>
#include <future>
#include <vector>

class CommunicationDevice;
class MainWindow;
class QPlainTextEdit;
class QTreeWidgetItem;
struct PortDescription;

class DeviceWorker : public QObject {
    Q_OBJECT
    public:
    ~DeviceWorker();

    void refresh_devices(QTreeWidgetItem *device_items, bool dut_only);

    void connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport);
    std::vector<PortDescription *> get_devices_with_protocol(const QString &protocol, const QStringList device_names);
    void set_currently_running_test(CommunicationDevice *com_device, const QString &test_name);
    QStringList get_string_list(ScriptEngine &script, const QString &name);

    bool is_dut_device(QTreeWidgetItem *item);
    bool is_device_in_use(QTreeWidgetItem *item);
    bool is_connected_to_device(QTreeWidgetItem *item);
	bool is_device_open(QTreeWidgetItem *item);
	void close_device(QTreeWidgetItem *item);
	void open_device(QTreeWidgetItem *item);

    private:
    void forget_device(QTreeWidgetItem *item);
    void detect_device(QTreeWidgetItem *item);
    void update_devices();
    void detect_devices();
	std::vector<PortDescription> communication_devices;
    bool contains_port(QMap<QString, QVariant> port_info);
    void detect_devices(std::vector<PortDescription *> device_list);
    DeviceMetaData device_meta_data;
    LIBUSBScan usbtmc_scan;
	QSemaphore refresh_semaphore{1};
    bool is_dut_device_(QTreeWidgetItem *item);
    bool is_device_in_use_(QTreeWidgetItem *item);

    bool is_connected_to_device_(QTreeWidgetItem *item);
	signals:
	void device_discovery_done();
};

#endif // DEVICEWORKER_H
