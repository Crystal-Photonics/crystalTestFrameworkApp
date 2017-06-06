#include "dataengineinput.h"

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
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
DataEngineInput::DataEngineInput(UI_container *parent, ScriptEngine *script_engine, Data_engine *data_engine, std::string field_id,
                                 std::string extra_explanation, std::string empty_value_placeholder, std::string desired_prefix, std::string actual_prefix)
    : parent{parent}
    , label_extra_explanation{new QLabel(parent)}
    , label_de_description{new QLabel(parent)}
    , label_de_desired_value{new QLabel(parent)}
    , label_de_actual_value{new QLabel(parent)}
    , label_ok{new QLabel(parent)}
    , timer{new QTimer(parent)}
    , data_engine{data_engine}
    , field_id{QString::fromStdString(field_id)}
    , empty_value_placeholder{QString::fromStdString(empty_value_placeholder)}
    , extra_explanation{QString::fromStdString(extra_explanation)}
    , desired_prefix{QString::fromStdString(desired_prefix)}
    , actual_prefix{QString::fromStdString(actual_prefix)}
    , script_engine{script_engine} {
    hlayout = new QHBoxLayout;

    hlayout->addWidget(label_extra_explanation, 0, Qt::AlignTop);
    hlayout->addWidget(label_de_description, 0, Qt::AlignTop);
    hlayout->addWidget(label_de_desired_value, 0, Qt::AlignTop);
    hlayout->addWidget(label_de_actual_value, 0, Qt::AlignTop);
    hlayout->addWidget(label_ok, 0, Qt::AlignTop);
    parent->add(hlayout, this);

    label_extra_explanation->setText(QString::fromStdString(extra_explanation));
    label_extra_explanation->setWordWrap(true);

    label_de_description->setText(data_engine->get_description(this->field_id));
    label_de_description->setWordWrap(true);
    label_de_desired_value->setText(this->desired_prefix + " " + data_engine->get_desired_value_as_string(this->field_id) + " " +
                                    data_engine->get_unit(this->field_id));
    label_de_desired_value->setWordWrap(true);
    label_de_actual_value->setText(this->actual_prefix + " " + this->empty_value_placeholder);
    label_de_actual_value->setWordWrap(true);
    label_ok->setText("-");
    if (!data_engine->is_desired_value_set(this->field_id)) {
        label_de_desired_value->setText(" ");
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

    auto *vlayout_next = new QVBoxLayout;
    auto *vlayout_yes = new QVBoxLayout;
    auto *vlayout_no = new QVBoxLayout;

    label_next = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence, "").toString() + ")", parent);
    button_next = new QPushButton("next", parent);
    vlayout_next->addWidget(button_next);
    vlayout_next->addWidget(label_next);
    vlayout_next->addStretch();

    label_yes = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence, "").toString() + ")", parent);
    button_yes = new QPushButton("Yes", parent);
    vlayout_yes->addWidget(button_yes);
    vlayout_yes->addWidget(label_yes);
    vlayout_yes->addStretch();

    label_no = new QLabel("(or " + QSettings{}.value(Globals::cancel_key_sequence, "").toString() + ")", parent);
    button_no = new QPushButton("No", parent);
    vlayout_no->addWidget(button_no);
    vlayout_no->addWidget(label_no);
    vlayout_no->addStretch();

    hlayout->addLayout(vlayout_next);
    hlayout->insertLayout(3, vlayout_no);
    hlayout->insertLayout(3, vlayout_yes);

    lineedit = new QLineEdit(parent);
    lineedit->setText(this->empty_value_placeholder);
    hlayout->insertWidget(3, lineedit, 0, Qt::AlignTop);

    auto sp_w = QSizePolicy::Maximum;
    auto sp_h = QSizePolicy::Maximum;
    label_extra_explanation->setSizePolicy(sp_w, sp_h);
    label_de_desired_value->setSizePolicy(sp_w, sp_h);
    label_de_actual_value->setSizePolicy(sp_w, sp_h);
    label_ok->setSizePolicy(sp_w, sp_h);
    button_yes->setSizePolicy(sp_w, sp_h);
    button_no->setSizePolicy(sp_w, sp_h);
    button_next->setSizePolicy(sp_w, sp_h);
    lineedit->setSizePolicy(sp_w, sp_h);

    callback_bool_no = QObject::connect(button_no, &QPushButton::clicked,
                                        [script_engine = this->script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::cancel_pressed); });
    callback_bool_yes = QObject::connect(button_yes, &QPushButton::clicked, [script_engine = this->script_engine]() {
        script_engine->hotkey_event_queue_send_event(HotKeyEvent::confirm_pressed);
    });

    callback_next = QObject::connect(button_next, &QPushButton::clicked,
                                     [script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent::confirm_pressed); });

    set_enabled(true);

    start_timer();

    parent->scroll_to_bottom();
    init_ok = true;
    set_ui_visibility();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

DataEngineInput::~DataEngineInput() {
    MainWindow::mw->execute_in_gui_thread([this] { //
        timer->stop();
        QObject::disconnect(callback_timer);
        QObject::disconnect(callback_next);
        QObject::disconnect(callback_bool_no);
        QObject::disconnect(callback_bool_yes);
        set_enabled(false);
    });
}
///\endcond

void DataEngineInput::load_actual_value() {
    is_editable = false;
    if (data_engine->value_complete(field_id)) {
        timer->stop();

        QString val = data_engine->get_actual_value(field_id);
        label_de_actual_value->setText(this->actual_prefix + " " + val + " " + data_engine->get_unit(this->field_id));
        if (data_engine->value_in_range(field_id)) {
            label_ok->setText("OK");
        } else {
            label_ok->setText("Fail");
        }
        set_ui_visibility();
    }
}

void DataEngineInput::set_editable() {
    if (is_editable) {
        return;
    }

    label_extra_explanation->setText(extra_explanation);

    is_editable = true;
    set_ui_visibility();
}

void DataEngineInput::save_to_data_engine() {
    if (is_editable) {
        auto entry_type = data_engine->get_entry_type(field_id);
        switch (entry_type) {
            case EntryType::Bool: {
                data_engine->set_actual_bool(field_id, bool_result.value());
            } break;
            case EntryType::Numeric: {
                QString t = lineedit->text();
                bool ok;
                double val = t.toDouble(&ok);
                if (ok) {
                    double si_prefix = data_engine->get_si_prefix(field_id);
                    data_engine->set_actual_number(field_id, val * si_prefix);
                } else {
                    throw std::runtime_error(QString("DataEngineInput line edit does not contain a number for field-id \"%1\"").arg(field_id).toStdString());
                }
            } break;
            case EntryType::String: {
                data_engine->set_actual_text(field_id, lineedit->text());
            } break;
            default:
                break;
        }
    } else {
        throw std::runtime_error(
            QString("DataEngineInput for field-id \"%1\" shall write to dataengine but since it is not in edit mode, there is no data to write.")
                .arg(field_id)
                .toStdString());
    }
    load_actual_value();
}

void DataEngineInput::sleep_ms(uint timeout_ms) {
    assert(MainWindow::gui_thread != QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    is_waiting = true;
    MainWindow::mw->execute_in_gui_thread([this] { //
        set_ui_visibility();

        if (!is_editable) {
            timer->start(BLINK_INTERVAL_MS);
        }
    });
    script_engine->timer_event_queue_run(timeout_ms);
    MainWindow::mw->execute_in_gui_thread([this] { //
        timer->stop();
        is_waiting = false;
        set_ui_visibility();
    });
}

void DataEngineInput::await_event() {
    //TODO: This now runs in the script thread, so we cannot call GUI functions like set_button_visibility directly anymore
    //use MainWindow::mw->execute_in_gui_thread to fix
    assert(MainWindow::gui_thread != QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    MainWindow::mw->execute_in_gui_thread([this] {              //
        is_waiting = true;
        switch (field_type) {
            case FieldType::Bool: {
                set_button_visibility(false, true);

            } break;
            case FieldType::Numeric:
            case FieldType::String: {
                set_button_visibility(true, false);

            } break;
            default:
                break;
        }
        if (!is_editable) {
            timer->start(BLINK_INTERVAL_MS);
        }
        set_ui_visibility();
    });
    auto result = script_engine->hotkey_event_queue_run();

    if (is_editable && (field_type == FieldType::Bool)) {
        if (result == HotKeyEvent::HotKeyEvent::confirm_pressed) {
            bool_result = true;
        } else {
            bool_result = false;
        }
    }
    MainWindow::mw->execute_in_gui_thread([this] {
        timer->stop();
        if (is_editable) {
            save_to_data_engine();
        }
        is_waiting = false;
        set_ui_visibility();
    });
}

void DataEngineInput::set_explanation_text(const std::string &extra_explanation) {
    this->extra_explanation = QString().fromStdString(extra_explanation);
    MainWindow::mw->execute_in_gui_thread([this] { label_extra_explanation->setText(this->extra_explanation); });
}

bool DataEngineInput::get_is_editable()
{
    return is_editable;
}

void DataEngineInput::set_visible(bool visible) {
    is_visible = visible;
    set_ui_visibility();
}

void DataEngineInput::set_enabled(bool enabled) {
    this->is_enabled = enabled;
    MainWindow::mw->execute_in_gui_thread([this, enabled] { //
        set_ui_visibility();
        if (enabled) {
            timer->start(BLINK_INTERVAL_MS);
        } else {
            timer->stop();
        }
    });
}

void DataEngineInput::clear_explanation() {
    MainWindow::mw->execute_in_gui_thread([this] { //
        label_extra_explanation->setText(" ");
    });
}
///\cond HIDDEN_SYMBOLS
void DataEngineInput::start_timer() {
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() {
        if (blink_state) {
            label_de_actual_value->setText(" ");
            label_extra_explanation->setText(" ");
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
        blink_state = !blink_state;

    });
}

void DataEngineInput::resizeEvent(QResizeEvent *event) {
    total_width = event->size().width();
    set_ui_visibility();
}

void DataEngineInput::set_total_visibilty() {
    MainWindow::mw->execute_in_gui_thread([this] { //
        label_extra_explanation->setVisible(is_visible && is_enabled && is_waiting);
        label_de_description->setVisible(is_visible);
        label_de_desired_value->setVisible(is_visible);
        label_de_actual_value->setVisible(is_visible && (is_editable == false));
        label_ok->setVisible(is_enabled && is_visible && (is_editable == false));
        lineedit->setVisible(is_visible && is_editable && is_enabled);
    });
}

void DataEngineInput::scale_columns() {
    MainWindow::mw->execute_in_gui_thread([this] { //
        int col_size = 6;
        if (is_editable && is_enabled && is_visible) {
            switch (field_type) {
                case FieldType::Numeric: {
                    lineedit->setFixedWidth((total_width / col_size));
                    button_next->setFixedWidth((total_width / col_size));
                } break;
                case FieldType::Bool: {
                    button_yes->setFixedWidth((total_width / (col_size * 2)));
                    button_no->setFixedWidth((total_width / (col_size * 2)));
                } break;
                case FieldType::String: {
                    lineedit->setFixedWidth((total_width / col_size));
                    button_next->setFixedWidth((total_width / col_size));
                } break;
                default:
                    break;
            }
            label_extra_explanation->setFixedWidth((total_width / col_size));
            //label_de_description->setFixedWidth((total_width / col_size));
            label_de_desired_value->setFixedWidth((total_width / col_size));

        } else if (is_enabled && is_visible) {
            label_extra_explanation->setFixedWidth((total_width / col_size));
            //label_de_description->setFixedWidth((total_width / col_size));
            label_de_desired_value->setFixedWidth((total_width / col_size));
            label_de_actual_value->setFixedWidth((total_width / col_size));
            label_ok->setFixedWidth((total_width / col_size));
            button_next->setFixedWidth((total_width / col_size));

        } else if ((is_enabled == false) && is_visible) {
            label_extra_explanation->setFixedWidth((total_width / col_size));
            //label_de_description->setFixedWidth((total_width / col_size));
            label_de_desired_value->setFixedWidth((total_width / col_size));
            label_de_actual_value->setFixedWidth((total_width / col_size));
            label_ok->setFixedWidth((total_width / col_size));
            button_next->setFixedWidth((total_width / col_size));

        } else if (!is_visible) {
        }
    });
}

void DataEngineInput::set_button_visibility(bool next, bool yes_no) {
    MainWindow::mw->execute_in_gui_thread([this, next, yes_no] {
        this->label_no->setVisible(yes_no);
        this->button_no->setVisible(yes_no);

        this->label_yes->setVisible(yes_no);
        this->button_yes->setVisible(yes_no);

        this->label_next->setVisible(next);
        this->button_next->setVisible(next);
    });
}

void DataEngineInput::set_labels_enabled() {
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

void DataEngineInput::set_ui_visibility() {
    MainWindow::mw->execute_in_gui_thread([this] {
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
        set_button_visibility(false, false);

        if (is_editable && is_enabled && is_visible) {
            switch (field_type) {
                case FieldType::Numeric: {
                    lineedit->setVisible(true);
                    auto validator = new QDoubleValidator(parent);
                    lineedit->setValidator(validator);

                } break;
                case FieldType::Bool: {
                    lineedit->setVisible(false);

                } break;
                case FieldType::String: {
                    lineedit->setVisible(true);
                    lineedit->setValidator(nullptr);

                } break;
                default:
                    break;
            }

        } else if (is_enabled && is_visible) {
            lineedit->setVisible(false);

        } else if ((is_enabled == false) && is_visible) {
            lineedit->setVisible(false);

        } else if (!is_visible) {
            lineedit->setVisible(false);
        }
    });
}

///\endcond
