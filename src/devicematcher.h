#ifndef DEVICEMATCHER_H
#define DEVICEMATCHER_H

#include "CommunicationDevices/comportcommunicationdevice.h"

#include "Protocols/protocol.h"
#include "deviceworker.h"
#include "testdescriptionloader.h"
#include "testrunner.h"
#include "util.h"

#include <QDialog>
#include <QList>
#include <vector>

struct ComportDescription {
    std::unique_ptr<ComportCommunicationDevice> device;
    QSerialPortInfo info;
    QTreeWidgetItem *ui_entry;
    std::unique_ptr<Protocol> protocol;
};

class DevicesToMatchEntry {
public:
    enum class MatchDefinitionState { OverDefined, UnderDefined, FullDefined };
    MatchDefinitionState match_definition = MatchDefinitionState::UnderDefined;
    DeviceRequirements device_requirement;
    std::vector<bool> selected_candidate;
    std::vector<std::pair<CommunicationDevice *, Protocol *>> accepted_candidates;
};

namespace Ui {
    class DeviceMatcher;
}

class DeviceMatcher : public QDialog {
    Q_OBJECT

    public:
    explicit DeviceMatcher(QWidget *parent = 0);
    ~DeviceMatcher();

    void match_devices(DeviceWorker &device_worker, TestRunner &runner, TestDescriptionLoader &test);

    std::vector<std::pair<CommunicationDevice *, Protocol *>> get_matched_devices();
    bool was_successful();

private slots:
    void on_DeviceMatcher_accepted();


    void on_tree_required_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    Ui::DeviceMatcher *ui;
    void make_gui();
    void load_available_devices(int required_index);
    QList<DevicesToMatchEntry> devices_to_match;
    //std::vector<std::pair<CommunicationDevice *, Protocol *>> device_matching_result;
    bool successful_matching = false;
    QString script_name;
};

#endif // DEVICEMATCHER_H
