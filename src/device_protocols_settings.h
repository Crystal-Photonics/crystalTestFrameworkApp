#ifndef DEVICE_PROTOCOLS_SETTINGS_H
#define DEVICE_PROTOCOLS_SETTINGS_H

#include <QString>
#include <QList>
#include <chrono>

enum class Color { red, green = 20, blue };

class DeviceProtocolSetting{
    public:
    using Duration = std::chrono::steady_clock::duration;
    enum type{comport, udp_connection, tcp_connection} type;
    int internet_port;
    QString ip_address;
    QString com_port_name_regex;
    int baud;
    QString escape;
    DeviceProtocolSetting::Duration timeout;
    DeviceProtocolSetting::Duration pause_after_discovery_flush;
};

class DeviceProtocolsSettings
{
public:
    DeviceProtocolsSettings(QString file_name);

    QList<DeviceProtocolSetting> protocols_rpc;
    QList<DeviceProtocolSetting> protocols_scpi;
private:
    void parse_settings_file(QString file_name);
    QString file_name;


};

#endif // DEVICE_PROTOCOLS_SETTINGS_H
