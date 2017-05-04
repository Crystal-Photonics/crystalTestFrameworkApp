#ifndef SCPIMETADATA_H
#define SCPIMETADATA_H

#include <QDate>
#include <QList>
#include <QString>

enum class DeviceMetaDataApprovedState { Unknown, Approved, Locked, Expired };

enum class ProtocolType { SCPI, Manual };

class DeviceMetaDataDetail {
    public:
    QString serial_number;
    QDate expery_date;
    QDate purchase_date;
    bool locked = false;
    bool valid = false;
    QString note;
    QString calibration_certificate_path;
    DeviceMetaDataApprovedState get_approved_state();
    QString get_approved_state_str();
};

class DeviceMetaDataCommon {
    public:
    QString  device_name;
    QString manual_path;
    QString manufacturer;
    QString description;
    ProtocolType protocol_type;
    void clear();
};

class DeviceMetaDataGroup {
    public:
    DeviceMetaDataCommon commondata;
    QList<DeviceMetaDataDetail> devices;
    void clear();
};

class DeviceEntry {
public:
    DeviceMetaDataCommon commondata;
    DeviceMetaDataDetail detail;
};

class DeviceMetaData {
    public:
    DeviceMetaData();
    void reload(QString file_name);
    DeviceMetaDataGroup query(QString serial_number, QString device_name);
    QList<DeviceEntry> get_manual_devices() const;

    private:
    void parse_meta_data_file(QString file_name);
    QList<DeviceMetaDataGroup> device_types;
};

#endif // SCPIMETADATA_H
