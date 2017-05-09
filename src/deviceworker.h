#ifndef DEVICEWORKER_H
#define DEVICEWORKER_H

#include "Protocols/protocol.h"
#include "qt_util.h"
#include "scriptengine.h"

#include "CommunicationDevices/usbtmc.h"
#include "scpimetadata.h"
#include <future>
#include <list>
#include <vector>
#include "CommunicationDevices/libusbscan.h"

class CommunicationDevice;
class MainWindow;
class QPlainTextEdit;
class QTreeWidgetItem;
struct PortDescription;

class DeviceWorker : public QObject {
    Q_OBJECT
    public:
    DeviceWorker();
    ~DeviceWorker();
    void forget_device(QTreeWidgetItem *item);
    void update_devices();
    void detect_devices();
    void detect_device(QTreeWidgetItem *item);
    void connect_to_device_console(QPlainTextEdit *console, CommunicationDevice *comport);
    std::vector<PortDescription *> get_devices_with_protocol(const QString &protocol, const QStringList device_names);
    void set_currently_running_test(CommunicationDevice *com_device, const QString &test_name);
    QStringList get_string_list(ScriptEngine &script, const QString &name);



    private:
    std::list<PortDescription> communication_devices;
    bool contains_port(QMap<QString, QVariant> port_info);
    void detect_devices(std::vector<PortDescription *> device_list);
    DeviceMetaData device_meta_data;
    LIBUSBScan usbtmc_scan;
};

#endif // DEVICEWORKER_H
