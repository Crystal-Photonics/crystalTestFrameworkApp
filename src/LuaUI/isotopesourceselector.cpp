#include "isotopesourceselector.h"
#include "config.h"
#include "ui_container.h"

#include <QComboBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>
#include <sol.hpp>

IsotopeSourceSelector::IsotopeSourceSelector(UI_container *parent)
	: combobox{new QComboBox(parent)} {
	combobox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	parent->add(combobox);
    load_isotope_database();
    fill_combobox_with_isotopes();
}

IsotopeSourceSelector::~IsotopeSourceSelector() {
    combobox->setEnabled(false);
    QObject::disconnect(callback_connection);
}

double IsotopeSourceSelector::get_selected_activity_Bq() {
    QString my_serial_number = combobox->currentText();
    QSettings{}.setValue(Globals::isotope_source_most_recent, my_serial_number);
    return get_source_by_serial_number(combobox->currentText()).get_activtiy_becquerel(QDate::currentDate());
}

void IsotopeSourceSelector::set_visible(bool visible)
{
    combobox->setVisible(visible);
}


std::string IsotopeSourceSelector::get_selected_serial_number() {
    return combobox->currentText().toStdString();
}

void IsotopeSourceSelector::fill_combobox_with_isotopes() {
    combobox->clear();
    for (auto item : isotope_sources) {
        combobox->addItem(item.serial_number);
    }
}

IsotopeSource IsotopeSourceSelector::get_source_by_serial_number(QString serial_number) {
    for (auto item : isotope_sources) {
        if (item.serial_number == serial_number) {
            return item;
        }
    }
    QString msg = QString{"cant find isotope source serial_number \"%1\""}.arg(serial_number);
    throw sol::error(msg.toStdString());
}

void IsotopeSourceSelector::load_isotope_database() {
    QString most_recent_serial_number = QSettings{}.value(Globals::isotope_source_most_recent, "").toString();
    QString fn = QSettings{}.value(Globals::isotope_source_data_base_path, "").toString();

    isotope_sources.clear();

    QFile loadFile(fn);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        QString msg = QString{"cant open isotope source file %1"}.arg(fn);
        throw sol::error(msg.toStdString());
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    QJsonObject obj = loadDoc.object();
    QJsonArray jarray = obj["isotopes"].toArray();
    for (auto item : jarray) {
        QJsonObject obj = item.toObject();
		if (obj["removed"].toString() == "yes") {
            continue;
        }
        IsotopeSource isotope_source;
        isotope_source.serial_number = obj["serial_number"].toString().trimmed();
        isotope_source.isotope = obj["isotope"].toString().trimmed();

        const QString DATE_FORMAT = "yyyy.M.d"; // 1985.5.25
        QString tmp = obj["start_date"].toString();

        isotope_source.start_date = QDate::fromString(tmp, DATE_FORMAT);

        isotope_source.start_activity_becquerel = obj["start_activity_kBq"].toDouble() * 1000;
        if ((isotope_source.isotope.toLower() == "co57") || (isotope_source.isotope.toLower() == "co-57")) {
            //271 Tage 17 Stunden
            isotope_source.half_time_days = 271 + 17 / 24;

        } else if ((isotope_source.isotope.toLower() == "na22") || (isotope_source.isotope.toLower() == "na-22")) {
            //2.6 Years
            isotope_source.half_time_days = 2.6 * 365;

        } else if ((isotope_source.isotope.toLower() == "j129") || (isotope_source.isotope.toLower() == "j-129")) {
            //1,57 * 10^7  years;
            isotope_source.half_time_days = 1.57e7 * 365;
        } else {
            QString msg = QString{"dont knwo halftime of isotope \"%1\" in isotope source database."}.arg(isotope_source.isotope);
            throw sol::error(msg.toStdString());
        }

        if (most_recent_serial_number == isotope_source.serial_number) {
            isotope_sources.insert(0, isotope_source);
        } else {
            isotope_sources.append(isotope_source);
        }
    }
}

///\cond HIDDEN_SYMBOLS
void IsotopeSourceSelector::set_single_shot_return_pressed_callback(std::function<void()> callback) {
#if 0
    callback_connection =
        QObject::connect(combobox, &QComboBox::returnPressed, [&callback_connection = this->callback_connection, callback = std::move(callback) ] {
            callback();
            QObject::disconnect(callback_connection);
        });
#endif
}

///\endcond

double IsotopeSource::get_activtiy_becquerel(QDate date_for_activity) {
    const double ln2 = 0.693147180559945;
    double days = start_date.daysTo(date_for_activity);

    double current_activity = (start_activity_becquerel * exp(-days * ln2 / half_time_days));

    return current_activity;
}
