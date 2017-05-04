#ifndef MANUALPROTOCOL_H
#define MANUALPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"
#include "Protocols/protocol.h"
#include "device_protocols_settings.h"
#include <QTreeWidgetItem>
#include "scpimetadata.h"

class ManualProtocol : public Protocol {

public:
    ManualProtocol();


    ~ManualProtocol();
    ManualProtocol(const ManualProtocol &) = delete;
    ManualProtocol(ManualProtocol &&other) = delete;
    ManualProtocol &operator=(const ManualProtocol &&) = delete;
    bool is_correct_protocol();
    void set_ui_description(QTreeWidgetItem *ui_entry);

    void set_meta_data(DeviceMetaDataGroup meta_data);

    std::string get_name();
    std::string get_manufacturer();
    std::string get_description();
    std::string get_serial_number();
    std::string get_notes();
    std::string get_summary();
    QString get_approved_state_str();
    DeviceMetaDataApprovedState get_approved_state();
private:
    DeviceMetaDataGroup meta_data;
    QString get_summary_lcl();

};

#endif // MANUALPROTOCOL_H
