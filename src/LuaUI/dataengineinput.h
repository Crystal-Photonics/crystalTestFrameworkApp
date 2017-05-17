#ifndef DATAENGINEINPUT_H
#define DATAENGINEINPUT_H

#include "data_engine/data_engine.h"
#include <QMetaObject>
#include <functional>
#include <string>

class QLabel;
class QSplitter;
class QTimer;
class QCheckBox;
class QHBoxLayout;
class UI_container;
class QWidget;
class QLineEdit;
/** \ingroup ui
 *  \{
 */
class DataEngineInput {
    public:
    DataEngineInput(UI_container *parent, Data_engine *data_engine, std::string field_id, std::string extra_explanation, std::string empty_value_placeholder,
                    std::__cxx11::string desired_prefix, std::__cxx11::string actual_prefix);
    ~DataEngineInput();

    void set_visible(bool visible);
    void set_enabled(bool enabled);
    void clear_explanation();
    void load_actual_value();
    void set_editable();
    void save_to_data_engine();

    private:
    UI_container *parent;

    QLabel *label_extra_explanation = nullptr;
    QLabel *label_de_description = nullptr;
    QLabel *label_de_desired_value = nullptr;
    QLabel *label_de_actual_value = nullptr;
    QLabel *label_ok = nullptr;
    QCheckBox *checkbox = nullptr;
    QLineEdit *lineedit = nullptr;
    QTimer *timer = nullptr;

    Data_engine *data_engine = nullptr;
    QHBoxLayout *hlayout = nullptr;
    QString field_id;

    QString empty_value_placeholder;
    QString extra_explanation;
    QString desired_prefix;
    QString actual_prefix;
    bool blink_state = false;
    bool editable = false;

    ///\cond HIDDEN_SYMBOLS
    void start_timer();
    ///\endcond

    QMetaObject::Connection callback_connection = {};
};

#endif // DATAENGINEINPUT_H
