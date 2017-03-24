#ifndef DEVICE_PROTOCOLS_SETTINGS_H
#define DEVICE_PROTOCOLS_SETTINGS_H

#include <QList>
#include <QString>
#include <chrono>

class DeviceProtocolSetting {
    public:
    using Duration = std::chrono::steady_clock::duration;
    enum type { comport, udp_connection, tcp_connection } type;

    void set_exclusive_port(QStringList *exclusive_ports);
    int internet_port;
    QString ip_address;
    QString com_port_name_regex;
    int baud;
    QString escape;
    DeviceProtocolSetting::Duration timeout;
    DeviceProtocolSetting::Duration pause_after_discovery_flush;

    bool match(QString port_name);

    private:
    QStringList *exclusive_ports;
};

class DeviceProtocolsSettings {
    public:
    DeviceProtocolsSettings(QString file_name);

    QList<DeviceProtocolSetting> protocols_rpc;
    QList<DeviceProtocolSetting> protocols_scpi;
    QList<DeviceProtocolSetting> protocols_sg04_count;

    private:
    void parse_settings_file(QString file_name);
    QString file_name;
    QStringList exclusive_ports;
};

#endif // DEVICE_PROTOCOLS_SETTINGS_H
