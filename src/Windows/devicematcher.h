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
    void set_is_in_use(bool in_use);
    bool get_is_in_use(void);

};

class CandidateEntry{
public:
    CommunicationDevice * communication_device;
    Protocol * protocol;
    bool selected = false;
};

class DevicesToMatchEntry {
    public:
    enum class MatchDefinitionState { OverDefined, UnderDefined, FullDefined };
    MatchDefinitionState match_definition = MatchDefinitionState::UnderDefined;
    DeviceRequirements device_requirement;
    std::vector<CandidateEntry> accepted_candidates;
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
    void on_tree_available_itemChanged(QTreeWidgetItem *item, int column);
    void on_btn_cancel_clicked();
    void on_btn_ok_clicked();

    void on_btn_check_all_clicked();

    void on_btn_uncheck_all_clicked();

    void poll_sg04_counts();
private:
    Ui::DeviceMatcher *ui;
    void make_treeview();
    void calc_gui_match_definition();
    void load_available_devices(int required_index);
    QList<DevicesToMatchEntry> devices_to_match;
    bool successful_matching = false;
    QString script_name;
    DevicesToMatchEntry *selected_requirement = nullptr;
    void calc_requirement_definitions();
    void align_columns();
};

#endif // DEVICEMATCHER_H
