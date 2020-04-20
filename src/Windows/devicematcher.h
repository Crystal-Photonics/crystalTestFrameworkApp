#ifndef DEVICEMATCHER_H
#define DEVICEMATCHER_H

#include "CommunicationDevices/comportcommunicationdevice.h"

#include "Protocols/protocol.h"
#include "deviceworker.h"
#include "testdescriptionloader.h"
#include "util.h"

#include <QDialog>
#include <QList>
#include <vector>

class TestRunner;

enum class CommunicationDeviceType { COM, TMC, Manual, TCP, UDP, IP };

QString ComDeviceTypeToString(CommunicationDeviceType t);

struct PortDescription {
    std::unique_ptr<CommunicationDevice> device;
    QMap<QString, QVariant> port_info;
	QTreeWidgetItem *ui_entry;
    std::unique_ptr<Protocol> protocol;
    CommunicationDeviceType communication_type; //COM, TMC, Manual, TCP, UDP, IP
};

class CandidateEntry {
    public:
    CommunicationDevice *communication_device;
    Protocol *protocol;
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

struct MatchedDevice {
	CommunicationDevice *device = nullptr;
	Protocol *protocol = nullptr;
    QString proposed_alias;
};

class DeviceMatcher : public QDialog {
    Q_OBJECT

    public:
	explicit DeviceMatcher(QWidget *parent = nullptr);
    ~DeviceMatcher();

	void match_devices(DeviceWorker &device_worker, TestRunner &runner, TestDescriptionLoader &test, const Sol_table &requirements);
	void match_devices(DeviceWorker &device_worker, TestRunner &runner, const std::vector<DeviceRequirements> &device_requirements, const QString &testname,
					   const Sol_table &requirements);
    bool is_match_possible(DeviceWorker &device_worker, TestDescriptionLoader &test);

    std::vector<MatchedDevice> get_matched_devices();
    bool was_successful();

    private slots:
    void on_DeviceMatcher_accepted();
    void on_tree_required_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_tree_available_itemChanged(QTreeWidgetItem *item, int column);
    void on_btn_cancel_clicked();
    void on_btn_ok_clicked();
    void on_btn_check_all_clicked();
    void on_btn_uncheck_all_clicked();

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
