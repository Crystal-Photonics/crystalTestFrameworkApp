#ifndef SCPIPROTOCOL_H
#define SCPIPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"
#include "protocol.h"
#include <chrono>
#include <memory>
#include <sol.hpp>

class QTreeWidgetItem;

struct SCPI_Device_Data {
    QString serial_number;
    QString manufacturer;
    QString name;
    QString version;

    QString get_summary() const;
    void get_lua_data(sol::table &t) const;

    private:
    struct Description_source {
        QString description;
        const QString &source;
    };
    std::vector<Description_source> get_description_source() const;
};

class SCPIProtocol : public Protocol {
    using Duration = std::chrono::steady_clock::duration;

    public:
    SCPIProtocol(CommunicationDevice &device);
    ~SCPIProtocol();
    SCPIProtocol(const SCPIProtocol &) = delete;
    SCPIProtocol(SCPIProtocol &&other) = delete;
    bool is_correct_protocol();
    void set_ui_description(QTreeWidgetItem *ui_entry);
    SCPIProtocol &operator=(const SCPIProtocol &&) = delete;
    void get_lua_device_descriptor(sol::table &t) const;
    CommunicationDevice::Duration default_duration = std::chrono::milliseconds{200};
    void clear();



    sol::table scpi_get_str(sol::state &lua, std::string request);
    sol::table scpi_get_str_param(sol::state &lua, std::string request, std::string argument);
    double scpi_get_num(std::string request);
    double scpi_get_num_param(std::string request, std::string argument);

    bool is_event_received(std::string event_name);
    void clear_event_list();
    sol::table get_event_list(sol::state &lua);

    std::string get_name(void);
    std::string get_serial_number(void);
    std::string get_manufacturer(void);

    private:
    QStringList scpi_get_str_param_raw( std::string request, std::string argument);
    QStringList scpi_parse_scpi_answers();
    QString scpi_parse_last_scpi_answer();
    void send_string(std::string data);
    QMetaObject::Connection connection;
    bool send_scpi_request(Duration timeout, std::string request, bool use_leading_escape);
    CommunicationDevice *device;
    SCPI_Device_Data device_data;
    QByteArray incoming_data;
    std::string escape_characters;
    std::string event_indicator = "*";
    void load_idn_string(std::string idn);
    QStringList event_list;
};

#endif // RPCPROTOCOL_H
