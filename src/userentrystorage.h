#ifndef UserEntryCache_H
#define UserEntryCache_H

#include <QJsonObject>
#include <QString>
class UserEntryCache {
    public:
    enum class AccessDirection { read_access, write_access};
    UserEntryCache();
    ~UserEntryCache();
    void load_storage_for_script(QString script_path, QString dut_id);
    QString get_value(QString field_name);
    void set_value(QString field_name, QString value);
    void clear();
    bool key_already_used(AccessDirection access_direction, QString field_name);

    private:
    QString dut_id_m;
    bool read_from_wild_card = false;
    bool modified_m = false;
    const QString wildcard = "*";
    QString file_name_m;
    QString script_path_m;
    QJsonObject cache_m;
};

#endif // UserEntryCache_H
