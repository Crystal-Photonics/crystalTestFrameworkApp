#include "polldataengine.h"

#include "Windows/mainwindow.h"
#include "config.h"
#include "ui_container.h"
#include "util.h"

#include <QCheckBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
PollDataEngine::PollDataEngine(UI_container *parent_, ScriptEngine *script_engine, Data_engine *data_engine_, std::string field_id_,
                                 std::string extra_explanation_, std::string empty_value_placeholder_, std::string desired_prefix_, std::string actual_prefix_)
    : UI_widget{parent_}
    , label_extra_explanation{new QLabel(parent_)}
    , label_de_description{new QLabel(parent_)}
    , label_de_desired_value{new QLabel(parent_)}
    , label_de_actual_value{new QLabel(parent_)}
    , label_ok{new QLabel(parent_)}
    , timer{new QTimer(parent_)}
    , data_engine{data_engine_}
    , field_id{QString::fromStdString(field_id_)}
    , empty_value_placeholder{QString::fromStdString(empty_value_placeholder_)}
    , extra_explanation{QString::fromStdString(extra_explanation_)}
    , desired_prefix{QString::fromStdString(desired_prefix_)}
    , actual_prefix{QString::fromStdString(actual_prefix_)}
    , script_engine{script_engine} {
#if 0
    qDebug() << "field_id" << QString::fromStdString(field_id_);
    qDebug() << "empty_value_placeholder_" << QString::fromStdString(empty_value_placeholder_);
    qDebug() << "extra_explanation_" << QString::fromStdString(extra_explanation_);
    qDebug() << "desired_prefix_" << QString::fromStdString(desired_prefix_);
    qDebug() << "actual_prefix_" << QString::fromStdString(actual_prefix_);
    qDebug() << "line" << __LINE__;
#endif
    hlayout = new QHBoxLayout;

    hlayout->addWidget(label_extra_explanation, 0, Qt::AlignTop);
    hlayout->addWidget(label_de_description, 0, Qt::AlignTop);
    hlayout->addWidget(label_de_desired_value, 0, Qt::AlignTop);
    hlayout->addWidget(label_de_actual_value, 0, Qt::AlignTop);
    hlayout->addWidget(label_ok, 0, Qt::AlignTop);
    parent->add(hlayout, this);

    label_extra_explanation->setText(extra_explanation);
    label_extra_explanation->setWordWrap(true);

    assert(data_engine);
    QString desc = data_engine->get_description(this->field_id);
    label_de_description->setText(desc);
    label_de_description->setWordWrap(true);
    QString s = this->desired_prefix + " " + data_engine->get_desired_value_as_string(this->field_id);
    label_de_desired_value->setText(s);
    label_de_desired_value->setWordWrap(true);
    label_de_actual_value->setText(this->actual_prefix + " " + this->empty_value_placeholder);
    label_de_actual_value->setWordWrap(true);
    label_ok->setText("-");
    if (!data_engine->is_desired_value_set(this->field_id)) {
        QString s = data_engine->get_unit(this->field_id);
        if (s.size()) {
            label_de_desired_value->setText("[" + s + "]");
        } else {
            label_de_desired_value->setText("");
        }
    }

    auto entry_type = data_engine->get_entry_type(this->field_id);
    switch (entry_type) {
        case EntryType::Bool: {
            field_type = FieldType::Bool;

        } break;
        case EntryType::Numeric: {
            field_type = FieldType::Numeric;
        } break;
        case EntryType::String: {
            field_type = FieldType::String;
        } break;
        default:
            assert(0);
            break;
    }







    auto sp_w = QSizePolicy::Maximum;
    auto sp_h = QSizePolicy::MinimumExpanding;
    //auto sp_h_f = QSizePolicy::Fixed;

    label_extra_explanation->setSizePolicy(sp_w, sp_h);
    label_de_desired_value->setSizePolicy(sp_w, sp_h);
    label_de_actual_value->setSizePolicy(sp_w, sp_h);
    label_ok->setSizePolicy(sp_w, sp_h);




    set_enabled(true);

    start_timer();

    parent->scroll_to_bottom();
    init_ok = true;
    set_ui_visibility();
    load_actual_value();

    assert(MainWindow::gui_thread == QThread::currentThread());
}

PollDataEngine::~PollDataEngine() {
    assert(MainWindow::gui_thread == QThread::currentThread());

    timer->stop();
    QObject::disconnect(callback_timer);

    set_enabled(false);
}
///\endcond

void PollDataEngine::load_actual_value() {
    is_editable = false;
    if (data_engine->value_complete_in_instance(field_id)) {
        timer->stop();

        QString val = data_engine->get_actual_value(field_id);
        label_de_actual_value->setText(this->actual_prefix + " " + val);
        if (data_engine->value_in_range_in_instance(field_id)) {
            label_ok->setText("OK");
        } else {
            label_ok->setText("Fail");
        }
        set_ui_visibility();
    }
}


void PollDataEngine::set_explanation_text(const std::string &extra_explanation) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    this->extra_explanation = QString().fromStdString(extra_explanation);
    label_extra_explanation->setText(this->extra_explanation);
}

void PollDataEngine::set_visible(bool visible) {
    is_visible = visible;
    set_ui_visibility();
}

void PollDataEngine::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    this->is_enabled = enabled;

    set_ui_visibility();
    if (enabled) {
        timer->start(BLINK_INTERVAL_MS);
    } else {
        timer->stop();
    }
}

///\cond HIDDEN_SYMBOLS
void PollDataEngine::start_timer() {
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() {
        if (blink_state == Globals::ui_blink_ratio) {
            label_de_actual_value->setText(" ");
            label_extra_explanation->setText(" ");
            blink_state = 0;
        } else {
            label_de_actual_value->setMinimumHeight(0);
            label_de_actual_value->setMaximumHeight(16777215);
            label_de_actual_value->setText(actual_prefix + " " + empty_value_placeholder);
            label_de_actual_value->setFixedHeight(label_de_actual_value->height());

            label_extra_explanation->setMinimumHeight(0);
            label_extra_explanation->setMaximumHeight(16777215);
            label_extra_explanation->setText(extra_explanation);
            label_extra_explanation->setFixedHeight(label_extra_explanation->height());
        }
        blink_state++;

    });
}

void PollDataEngine::resizeMe(QResizeEvent *event) {
    total_width = event->size().width();
    set_ui_visibility();
}

void PollDataEngine::set_total_visibilty() {
    assert(MainWindow::gui_thread == QThread::currentThread());

    label_extra_explanation->setVisible(is_visible && is_enabled && is_waiting);
    label_de_description->setVisible(is_visible);
    label_de_desired_value->setVisible(is_visible);
    label_de_actual_value->setVisible(is_visible && (is_editable == false));
    label_ok->setVisible(is_visible && ((is_editable == false) || (is_enabled == false)));

    //    });
}

void PollDataEngine::scale_columns() {
    assert(MainWindow::gui_thread == QThread::currentThread());

    int col_size = 7;
    if (is_editable && is_enabled && is_visible) {

        label_extra_explanation->setFixedWidth((total_width / col_size));
        //label_de_description->setFixedWidth((total_width / col_size));
        label_de_desired_value->setFixedWidth((total_width / col_size));
    } else if (is_enabled && is_visible) {
        label_extra_explanation->setFixedWidth((total_width / col_size));
        //label_de_description->setFixedWidth((total_width / col_size));
        label_de_desired_value->setFixedWidth((total_width / col_size));
        label_de_actual_value->setFixedWidth((total_width / col_size));
        label_ok->setFixedWidth((total_width / col_size));


    } else if ((is_enabled == false) && is_visible) {
        label_extra_explanation->setFixedWidth((total_width / col_size));
        //label_de_description->setFixedWidth((total_width / col_size));
        label_de_desired_value->setFixedWidth((total_width / col_size));
        label_de_actual_value->setFixedWidth((total_width / col_size));
        label_ok->setFixedWidth((total_width / col_size));

    } else if (!is_visible) {
    }
}


void PollDataEngine::set_labels_enabled() {
    label_extra_explanation->setEnabled(is_enabled);
    label_de_description->setEnabled(is_enabled);
    label_de_desired_value->setEnabled(is_enabled);
    label_de_actual_value->setEnabled(is_enabled);
    if (is_enabled == false) {
        //label_extra_explanation->setText(" ");
        label_extra_explanation->setVisible(false);
    } else {
        // label_extra_explanation->setText(extra_explanation);
        label_extra_explanation->setVisible(true);
    }
    label_ok->setEnabled(is_enabled);
}

void PollDataEngine::set_ui_visibility() {
    Utility::promised_thread_call(MainWindow::mw, [this] {

        if (init_ok == false) {
            return;
        }
        scale_columns();
        if (is_waiting) {
            label_ok->setVisible(false);
            label_ok->setText(empty_value_placeholder);
            label_extra_explanation->setVisible(is_visible && is_enabled && is_waiting);
            return;
        }
        set_labels_enabled();
        set_total_visibilty();


    });
}

///\endcond
