#ifndef POLLDATAENGINE_H
#define POLLDATAENGINE_H

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
class QGridLayout;
class QWidget;
class QLineEdit;
class QPushButton;

/** \ingroup ui
 *  \{
 */
class FieldEntry {
public:
    enum FieldType { Bool, String, Numeric };
    QString field_id;
    QLabel *label_de_description = nullptr;
    QLabel *label_de_desired_value = nullptr;
    QLabel *label_de_actual_value = nullptr;
    QLabel *label_ok = nullptr;
    FieldType field_type;
};

class PollDataEngine : public UI_widget {


    public:
    PollDataEngine(UI_container *parent_, ScriptEngine *script_engine, Data_engine *data_engine_, QStringList items);
    ~PollDataEngine();

    void set_visible(bool visible);
    void set_enabled(bool is_enabled);
    void refresh();
    bool is_in_range();

    private:
    void load_actual_value();

    QTimer *timer = nullptr;

    QList<FieldEntry> field_entries;
    Data_engine *data_engine = nullptr;
    QGridLayout *grid_layout = nullptr;

    QString empty_value_placeholder{"/"};

    bool is_enabled = true;
    bool is_visible = true;
    ///\cond HIDDEN_SYMBOLS
    void start_timer();
    ///\endcond
    ScriptEngine *script_engine;
    void resizeMe(QResizeEvent *event) override;

    QMetaObject::Connection callback_timer = {};
    void set_ui_visibility();

    void set_labels_enabled();
    void set_total_visibilty();
    const int BLINK_INTERVAL_MS = 500;
    bool init_ok = false;

};

#endif // DATAENGINEINPUT_H
