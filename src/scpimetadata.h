#ifndef SCPIMETADATA_H
#define SCPIMETADATA_H

#include <QString>
#include <QDate>
#include <QList>

enum class DeviceMetaDataApprovedState{
    Unknown,
    Approved,
    Locked,
    Expired
};

enum class ProtocolType{
    SCPI,Manual
};

class DeviceMetaDataDetail{
public:
    QString serial_number;
    QDate expery_date;
    QDate purchase_date;
    bool locked=false;
    bool valid = false;
    QString note;
    QString calibration_certificate_path;
    DeviceMetaDataApprovedState get_approved_state();
    QString get_approved_state_str();

};

class DeviceMetaDataGroup{
public:
    QString device_name;
    QString manual_path;
    QString manufacturer;
    QString description;
    ProtocolType protocol_type;
    QList<DeviceMetaDataDetail> devices;
    void clear();
};

class DeviceMetaData
{
public:
    DeviceMetaData();
    void reload(QString file_name);
    DeviceMetaDataGroup query(QString serial_number, QString device_name);
private:

    void parse_meta_data_file(QString file_name);
    QList<DeviceMetaDataGroup> device_types;
};

#endif // SCPIMETADATA_H
