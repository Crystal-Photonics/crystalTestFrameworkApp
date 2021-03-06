#ifndef SCPIPROTOCOL_H
#define SCPIPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"
#include "device_protocols_settings.h"
#include "protocol.h"
#include "scpimetadata.h"
#include <chrono>
#include <memory>
#include <sol_forward.hpp>

class QTreeWidgetItem;

struct SCPI_Device_Data {
    QString serial_number;
    QString manufacturer;
    QString name;
    QString version;
    QDate expery_date;
    QDate purchase_date;
    bool locked = false;
    //bool metadata_valid;
    QString manual_path;
    QString calibration_certificate_path;
    QString note;

    QString get_summary();
    void get_lua_data(sol::table &t);

    DeviceMetaDataApprovedState get_approved_state();
    QString get_approved_state_str();

    void set_approved_state(DeviceMetaDataApprovedState as, QString sr);

    private:
    struct Description_source {
        QString name;
        QString description;
        const QString source;
    };
    DeviceMetaDataApprovedState approved_state;
    QString approved_str;
    std::vector<Description_source> get_description_source();
};

class SCPIProtocol : public Protocol {
    using Duration = std::chrono::steady_clock::duration;

    public:
    SCPIProtocol(CommunicationDevice &device, DeviceProtocolSetting setting);
    ~SCPIProtocol();
    SCPIProtocol(const SCPIProtocol &) = delete;
    SCPIProtocol(SCPIProtocol &&other) = delete;
    SCPIProtocol &operator=(const SCPIProtocol &&) = delete;
    bool is_correct_protocol();
    void set_ui_description(QTreeWidgetItem *ui_entry);

    void get_lua_device_descriptor(sol::table &t);
    DeviceMetaDataApprovedState get_approved_state();
    QString get_approved_state_str();
    QString get_device_summary();
    void clear();
    void set_scpi_meta_data(DeviceMetaDataGroup scpi_meta_data);

    sol::table get_str(sol::state &lua, std::string request);
    sol::table get_str_param(sol::state &lua, std::string request, std::string argument);
    double get_num(std::string request);
    double get_num_param(std::string request, std::string argument);

    void send_command(std::string request);

    bool is_event_received(std::string event_name);
    void clear_event_list();
    sol::table get_event_list(sol::state &lua);

    std::string get_name(void);
    std::string get_serial_number(void);
    std::string get_manufacturer(void);
    QString get_manual() const override;

    void set_validation_retries(unsigned int retries);
    void set_validation_max_standard_deviation(double max_std_dev);

    private:
    QStringList get_str_param_raw(std::string request, std::string argument);
    QStringList parse_scpi_answers();
    QString parse_last_scpi_answer();
    [[noreturn]] void throw_connection_error(const std::string &request);

    void send_string(std::string data);
    QMetaObject::Connection connection;
    bool send_scpi_request(Duration timeout, std::string request, bool use_leading_escape, bool answer_expected);
    CommunicationDevice *device;
    SCPI_Device_Data device_data;
    QByteArray incoming_data;
    std::string escape_characters;
    std::string event_indicator = "*";
    void load_idn_string(std::string idn);
    QStringList event_list;

    int retries_per_transmission{2};
    double maximal_acceptable_standard_deviation = 0.1;
    DeviceProtocolSetting device_protocol_setting;
};

#endif // RPCPROTOCOL_H
