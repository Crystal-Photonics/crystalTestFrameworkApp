#ifndef ISOTOPESOURCESELECTOR_H
#define ISOTOPESOURCESELECTOR_H

#include <QSplitter>
#include <QComboBox>
#include <QMetaObject>
#include <functional>
#include <string>
#include <QList>
#include <QDateTime>

class IsotopeSource{
public:
    QString serial_number;
    QString isotope;
    QString normal_user;
    QDate start_date;
    double start_activity_becquerel;

    double half_time_days;
    double get_activtiy_becquerel(QDate date_for_activity);
};

class IsotopeSourceSelector
{
public:
    IsotopeSourceSelector(QSplitter *parent);

    ~IsotopeSourceSelector();

    double get_selected_activity_Bq();
    std::string get_selected_serial_number();

    private:
    void load_isotope_database();
    QComboBox *combobox = nullptr;
    QMetaObject::Connection callback_connection = {};
    void set_single_shot_return_pressed_callback(std::function<void()> callback);

    void fill_combobox_with_isotopes();
    QList<IsotopeSource> isotope_sources;
    IsotopeSource get_source_by_serial_number(QString serial_number);

};
#endif // ISOTOPESOURCESELECTOR_H
