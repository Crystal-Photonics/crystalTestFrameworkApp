#ifndef SG04COUNTPROTOCOL_H
#define SG04COUNTPROTOCOL_H
#include "CommunicationDevices/communicationdevice.h"
#include "Protocols/protocol.h"
#include "device_protocols_settings.h"
#include <QTreeWidgetItem>
#include <sol.hpp>
#include <Qtime>

class SG04CountProtocol : public Protocol {
    public:
    SG04CountProtocol(CommunicationDevice &device, DeviceProtocolSetting &setting);
    ~SG04CountProtocol();
    SG04CountProtocol(const SG04CountProtocol &) = delete;
    SG04CountProtocol(SG04CountProtocol &&other) = delete;
    SG04CountProtocol &operator=(const SG04CountProtocol &&) = delete;
    bool is_correct_protocol();
    void set_ui_description(QTreeWidgetItem *ui_entry);


    void sg04_counts_clear();
    sol::table get_sg04_counts(sol::state &lua, bool clear);
    int await_count_package();

    uint16_t get_actual_count_rate();
    unsigned int get_actual_count_rate_cps();
    private:
    QMetaObject::Connection connection;
    CommunicationDevice *device;
    DeviceProtocolSetting device_protocol_setting;
    QByteArray incoming_data;
    uint16_t actual_count_rate;

    uint32_t received_counts = 0;

    QList<uint16_t> received_count_packages;
    QTime performance_measurement_timer;
};

#endif // SG04COUNTPROTOCOL_H
