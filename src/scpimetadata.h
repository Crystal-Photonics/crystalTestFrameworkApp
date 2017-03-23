#ifndef SCPIMETADATA_H
#define SCPIMETADATA_H

#include <QString>
#include <QDate>
#include <QList>

enum class SCPIApprovedState{
    Unknown,
    Approved,
    Locked,
    Expired
};

class SCPIDeviceDetail{
public:
    QString serial_number;
    QDate expery_date;
    QDate purchase_date;
    bool locked=false;
    bool valid = false;
    QString note;
    QString calibration_certificate_path;
    SCPIApprovedState get_approved_state();
    QString get_approved_state_str();
};

class SCPIDeviceType{
public:
    QString device_name;
    QString manual_path;
    QList<SCPIDeviceDetail> devices;
    void clear();
};

class SCPIMetaData
{
public:
    SCPIMetaData();
    void reload(QString file_name);
    SCPIDeviceType query(QString serial_number, QString device_name);
private:

    void parse_meta_data_file(QString file_name);
    QList<SCPIDeviceType> device_types;
};

#endif // SCPIMETADATA_H
