#ifndef DATAENGINEINPUT_H
#define DATAENGINEINPUT_H

#include "data_engine/data_engine.h"
#include "scriptengine.h"
#include "ui_container.h"
#include <QMetaObject>
#include <functional>
#include <string>

class QLabel;
class QSplitter;
class QTimer;
class QCheckBox;
class QHBoxLayout;
class QWidget;
class QLineEdit;
class QPushButton;

/** \ingroup ui
 *  \{
 */
class DataEngineInput : public UI_widget {
    enum FieldType { Bool, String, Numeric };

    public:
    DataEngineInput(UI_container *parent_, ScriptEngine *script_engine, Data_engine *data_engine_, std::string field_id_, std::string extra_explanation,
                    std::string empty_value_placeholder, std::string desired_prefix, std::string actual_prefix_);
    ~DataEngineInput();

    void set_visible(bool visible);
    void set_enabled(bool is_enabled);
    void clear_explanation();
    void load_actual_value();
    void set_editable();
    void save_to_data_engine();
    void await_event();
    void sleep_ms(uint timeout_ms);
    void set_explanation_text(const std::string &extra_explanation);

    bool get_is_editable();
    private:


    QLabel *label_extra_explanation = nullptr;
    QLabel *label_de_description = nullptr;
    QLabel *label_de_desired_value = nullptr;
    QLabel *label_de_actual_value = nullptr;
    QLabel *label_ok = nullptr;

    QPushButton *button_yes = nullptr;
    QLabel *label_yes = nullptr;

    QPushButton *button_no = nullptr;
    QLabel *label_no = nullptr;

    QPushButton *button_next = nullptr;
    QLabel *label_next = nullptr;

    QLineEdit *lineedit = nullptr;
    QTimer *timer = nullptr;

    Data_engine *data_engine = nullptr;
    QHBoxLayout *hlayout = nullptr;
    QString field_id;

    QString empty_value_placeholder;
    QString extra_explanation;
    QString desired_prefix;
    QString actual_prefix;
    uint blink_state = 0;
    bool is_editable = false;

    bool is_enabled = true;
    bool is_visible = true;
    bool is_waiting = false;
    ///\cond HIDDEN_SYMBOLS
    void start_timer();
    ///\endcond
    ScriptEngine *script_engine;
    void resizeMe(QResizeEvent *event) override;
    std::experimental::optional<bool> bool_result;

    QMetaObject::Connection callback_timer = {};
    QMetaObject::Connection callback_bool_yes = {};
    QMetaObject::Connection callback_bool_no = {};
    QMetaObject::Connection callback_next = {};
    void set_ui_visibility();
    void set_button_visibility(bool next, bool yes_no);
    FieldType field_type;
    void set_labels_enabled();
    void set_total_visibilty();
    const int BLINK_INTERVAL_MS = 500;
    uint total_width = 0;
    bool init_ok = false;

    void scale_columns();
};

#endif // DATAENGINEINPUT_H
