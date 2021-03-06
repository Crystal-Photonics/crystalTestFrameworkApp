#include "isotopesourceselector.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "qt_util.h"
#include "ui_container.h"
#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <sol.hpp>

/** \defgroup configuration Configuration files
  Description of the configuration files.
  \{
  \defgroup radioactive_source_database Isotope Source Database
    \{
    # Introduction
    The radioactive source database is used to list all sources with all
necessary data for calculating their current activity. You can access the
database using the class \c IsotopeSourceSelector.

    # The Database Structure
    The database is organized as a simple \glos{json} file.
    \par
    The file contains the object "isotopes" which is an array. In that array
each radioactive source is listed as an element with the fields: \li \c
serial_number: string, the serialnumber of the radioactive source. e.g.: AF4458
    \li \c isotope: string, the isotope name. The isotope name is used to
calculate the half-time. see also: \ref
IsotopeSourceSelector::get_selected_activity_Bq())<br> Currently supported
isotopes:
        - co57
        - i129
        - na22
    \li \c start_date: string, the calibration date of the source. Format:
"YYYY.MM.DD". \li \c start_activity_kBq: number, the activity of the source at
\c start_date \li \c removed: string, if the value is "yes" the radioactive
source will be hidden to the user.
        - yes
        - no



    # The Database Location
    The location of the file must be configured via the menu:

    Tools / Settings / Path settings / "Path to isotope source database file"

    \par example:
    \code{.json}
        {
            "isotopes":[
            {
                "serial_number": "AF4458",
                "isotope": "co57",
                "start_activity_kBq": 191,
                "start_date":"2015.03.15",
                "removed":"no"
            },{
                "serial_number": "464-83",
                "isotope": "i129",
                "start_activity_kBq": 37,
                "start_date":"1994.6.30",
                "removed":"no"
            },{
                "serial_number": "RG319",
                "isotope": "na22",
                "start_activity_kBq": 370,
                "start_date":"2008.7.9",
                "removed":"no"
            }
            ]
        }
    \endcode


    \} //end of group radioactive_source_database
\} */ // end of group configuration

/// \cond HIDDEN_SYMBOLS
IsotopeSourceSelector::IsotopeSourceSelector(UI_container *parent)
    : UI_widget{parent}
    , combobox{new QComboBox(parent)} {
    combobox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QVBoxLayout *layout = new QVBoxLayout;
    QLabel *label = new QLabel(parent);
    label->setText(" ");
    layout->addWidget(label);
    layout->addWidget(combobox);
    parent->add(layout, this);
    load_isotope_database();
    fill_combobox_with_isotopes("*");
    parent->scroll_to_bottom();
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
}

IsotopeSourceSelector::~IsotopeSourceSelector() {
    combobox->setEnabled(false);
    disconnect_isotope_selecteted();
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
}

double IsotopeSourceSelector::get_selected_activity_Bq() {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    QString my_serial_number = combobox->currentText();
    save_most_recent();
    return get_source_by_serial_number(combobox->currentText()).get_activtiy_becquerel(QDate::currentDate());
}

void IsotopeSourceSelector::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    combobox->setVisible(visible);
}

void IsotopeSourceSelector::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    combobox->setEnabled(enabled);
}

void IsotopeSourceSelector::filter_by_isotope(std::string isotope_name) {
    isotope_filter_m = QString::fromStdString(isotope_name);
    fill_combobox_with_isotopes(QString::fromStdString(isotope_name));
}

std::string IsotopeSourceSelector::get_selected_serial_number() {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    return combobox->currentText().toStdString();
}

std::string IsotopeSourceSelector::get_selected_name() {
    auto isot = get_source_by_serial_number(QString::fromStdString(get_selected_serial_number()));
    return isot.isotope.toStdString();
}

void IsotopeSourceSelector::fill_combobox_with_isotopes(QString isotope_name) {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    disconnect_isotope_selecteted();
    combobox->clear();
    for (auto item : isotope_sources) {
        if ((isotope_name == "") || (isotope_name == "*") || (item.isotope.toLower() == isotope_name.toLower())) {
            combobox->addItem(item.serial_number);
        }
    }
    load_most_recent();
    connect_isotope_selecteted();
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
    QString fn = QSettings{}.value(Globals::isotope_source_data_base_path_key, "").toString();

    isotope_sources.clear();

    if ((fn == "") || !QFile::exists(fn)) {
        QString msg = QString{"isotope source file %1 does not exist."}.arg(fn);
        throw sol::error(msg.toStdString());
    }

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
            // 271 Tage 17 Stunden
            isotope_source.half_time_days = 271 + 17 / 24;

        } else if ((isotope_source.isotope.toLower() == "na22") || (isotope_source.isotope.toLower() == "na-22")) {
            // 2.6 Years
            isotope_source.half_time_days = 2.6 * 365;

        } else if ((isotope_source.isotope.toLower() == "j129") || (isotope_source.isotope.toLower() == "j-129") ||
                   (isotope_source.isotope.toLower() == "i129") || (isotope_source.isotope.toLower() == "i-129")) {
            // 1,57 * 10^7  years;
            isotope_source.half_time_days = 1.57e7 * 365;
        } else {
            QString msg =
                QString{
                    "dont knwo halftime of isotope \"%1\" in isotope "
                    "source database."}
                    .arg(isotope_source.isotope);
            throw sol::error(msg.toStdString());
        }
        isotope_sources.append(isotope_source);
    }
}

void IsotopeSourceSelector::load_most_recent() {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    QString most_recent_serial_number = QSettings{}.value(QString(Globals::isotope_source_most_recent_key) + "_" + isotope_filter_m.toLower(), "").toString();
    if (combobox->findText(most_recent_serial_number) > -1) {
        combobox->setCurrentText(most_recent_serial_number);
    }
}

void IsotopeSourceSelector::save_most_recent() {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    QSettings{}.setValue(QString(Globals::isotope_source_most_recent_key) + "_" + isotope_filter_m.toLower(), combobox->currentText());
}

void IsotopeSourceSelector::connect_isotope_selecteted() {
    assert(MainWindow::gui_thread == QThread::currentThread()); // event_queue_run_ must not be started by the
                                                                // GUI-thread because it would freeze the GUI
    assert(!callback_isotope_selected);
    callback_isotope_selected = QObject::connect(combobox, &QComboBox::currentTextChanged, [this](const QString &text) {
        save_most_recent();
        (void)text;
    });
}

void IsotopeSourceSelector::disconnect_isotope_selecteted() {
    if (callback_isotope_selected) {
        QObject::disconnect(callback_isotope_selected);
    }
}

void IsotopeSourceSelector::set_single_shot_return_pressed_callback(std::function<void()> callback) {
#if 0
    callback_connection =
        QObject::connect(combobox, &QComboBox::returnPressed, [&callback_connection = this->callback_connection, callback = std::move(callback) ] {
            callback();
            QObject::disconnect(callback_connection);
        });
#else
    (void)callback;
#endif
}

double IsotopeSource::get_activtiy_becquerel(QDate date_for_activity) {
    const double ln2 = 0.693147180559945;
    double days = start_date.daysTo(date_for_activity);

    double current_activity = (start_activity_becquerel * std::exp(-days * ln2 / half_time_days));

    return current_activity;
}
/// \endcond
