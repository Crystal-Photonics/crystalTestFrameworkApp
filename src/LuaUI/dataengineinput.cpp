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
DataEngineInput::DataEngineInput(UI_container *parent_, ScriptEngine *script_engine, Data_engine *data_engine_, std::string field_id_,
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

    auto *vlayout_next = new QVBoxLayout;
    auto *vlayout_yes = new QVBoxLayout;
    auto *vlayout_no = new QVBoxLayout;
    auto *vlayout_ea = new QVBoxLayout;

    label_next = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence_key, "").toString() + ")", parent_);
    button_next = new QPushButton(QObject::tr("Next"), parent_);
    vlayout_next->addWidget(button_next);
    vlayout_next->addWidget(label_next);
    vlayout_next->addStretch();

    label_yes = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence_key, "").toString() + ")", parent_);
    button_yes = new QPushButton(QObject::tr("Yes"), parent_);
    vlayout_yes->addWidget(button_yes);
    vlayout_yes->addWidget(label_yes);
    vlayout_yes->addStretch();

    label_no = new QLabel("(or " + QSettings{}.value(Globals::cancel_key_sequence_key, "").toString() + ")", parent_);
    button_no = new QPushButton(QObject::tr("No"), parent_);
    vlayout_no->addWidget(button_no);
    vlayout_no->addWidget(label_no);
    vlayout_no->addStretch();

    label_exceptional_approval = new QLabel(" ", parent_);
    button_exceptional_approval = new QPushButton(QObject::tr("Exceptional approval"), parent_);
    vlayout_ea->addWidget(button_exceptional_approval);
    vlayout_ea->addWidget(label_exceptional_approval);
    vlayout_ea->addStretch();

    hlayout->insertLayout(3, vlayout_ea);
    hlayout->insertLayout(3, vlayout_next);
    hlayout->insertLayout(3, vlayout_no);
    hlayout->insertLayout(3, vlayout_yes);

    lineedit = new QLineEdit(parent);
    lineedit->setText(this->empty_value_placeholder);
    hlayout->insertWidget(3, lineedit, 0, Qt::AlignTop);

    auto sp_w = QSizePolicy::Maximum;
    auto sp_h = QSizePolicy::MinimumExpanding;
    //auto sp_h_f = QSizePolicy::Fixed;

    label_extra_explanation->setSizePolicy(sp_w, sp_h);
    label_de_desired_value->setSizePolicy(sp_w, sp_h);
    label_de_actual_value->setSizePolicy(sp_w, sp_h);
    label_ok->setSizePolicy(sp_w, sp_h);
    button_yes->setSizePolicy(sp_w, sp_h);
    button_no->setSizePolicy(sp_w, sp_h);
    button_next->setSizePolicy(sp_w, sp_h);
    button_exceptional_approval->setSizePolicy(sp_w, sp_h);
    lineedit->setSizePolicy(sp_w, sp_h);

    //   label_exceptional_approval->setSizePolicy(sp_w, sp_h);
    label_exceptional_approval->setText(" ");
    //    label_exceptional_approval->setFixedHeight(label_exceptional_approval->height());
    // label_exceptional_approval->setText(" ");
    // button_exceptional_approval->setSizePolicy(sp_w, sp_h_f);

    // button_yes->setFixedHeight(button_exceptional_approval->height());
    //  button_no->setFixedHeight(button_exceptional_approval->height());
    //  button_next->setFixedHeight(button_exceptional_approval->height());

    callback_bool_no = QObject::connect(button_no, &QPushButton::clicked,
                                        [script_engine = this->script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::cancel_pressed); });
    callback_bool_yes = QObject::connect(button_yes, &QPushButton::clicked, [script_engine = this->script_engine]() {
        script_engine->hotkey_event_queue_send_event(HotKeyEvent::confirm_pressed);
    });

    callback_next = QObject::connect(button_next, &QPushButton::clicked,
                                     [script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent::confirm_pressed); });

    callback_exceptional_approval = QObject::connect(button_exceptional_approval, &QPushButton::clicked, [this, script_engine]() {
        ExceptionalApprovalDB ea_db{QSettings{}.value(Globals::path_to_excpetional_approval_db_key, "").toString()};
        if (data_engine->do_exceptional_approval(ea_db, field_id, MainWindow::mw)) {
            if ((field_type == FieldType::Numeric) || (field_type == FieldType::String)) {
                if (lineedit->text().count() == 0) {
                    dont_save_result_to_de = true;
                }
            }

            script_engine->hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent::skip_pressed);
        }
    });

    set_enabled(true);

    start_timer();

    parent->scroll_to_bottom();
    init_ok = true;
    set_ui_visibility();
    load_actual_value();
    if ((field_type == FieldType::Numeric) || (field_type == FieldType::String)) {
        lineedit->setFocus();
    }
    assert(MainWindow::gui_thread == QThread::currentThread());
}

DataEngineInput::~DataEngineInput() {
    assert(MainWindow::gui_thread == QThread::currentThread());

    timer->stop();
    QObject::disconnect(callback_timer);
    QObject::disconnect(callback_next);
    QObject::disconnect(callback_bool_no);
    QObject::disconnect(callback_bool_yes);
    set_enabled(false);
}
///\endcond

void DataEngineInput::load_actual_value() {
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

void DataEngineInput::set_editable() {
    if (is_editable) {
        return;
    }

    label_extra_explanation->setText(extra_explanation);

    is_editable = true;
    set_ui_visibility();
}

void DataEngineInput::save_to_data_engine() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
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
                double si_prefix = data_engine->get_si_prefix(field_id);
                if (!ok) {
                    val = QInputDialog::getDouble(parent, "Invalid value", "Der Wert \"" + t + "\" im Feld \"" + label_de_description->text() + " " +
                                                                               label_de_desired_value->text() +
                                                                               "\" ist keine Nummer. Bitte tragen Sie die nach.");

#if 0
                    auto s = QString("DataEngineInput line edit does not contain a number for field-id \"%1\"").arg(field_id);
                    qDebug() << s;
                    throw std::runtime_error(s.toStdString());
#endif
                }

                data_engine->set_actual_number(field_id, val * si_prefix);

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
    set_ui_visibility();
    Utility::promised_thread_call(MainWindow::mw, [this] {

        if (!is_editable) {
            timer->start(BLINK_INTERVAL_MS);
        }
    });
    script_engine->timer_event_queue_run(timeout_ms);
    Utility::promised_thread_call(MainWindow::mw, [this] {
        timer->stop();
        is_waiting = false;
    });
    set_ui_visibility();
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw sol::error("Abort Requested");
    }
}

void DataEngineInput::await_event() {
    assert(MainWindow::gui_thread != QThread::currentThread());
    dont_save_result_to_de = false;
    Utility::promised_thread_call(MainWindow::mw, [this] {
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
        } else if (result == HotKeyEvent::HotKeyEvent::cancel_pressed) {
            bool_result = false;
        } else {
            dont_save_result_to_de = true;
        }
    }
    is_waiting = false;

    Utility::promised_thread_call(MainWindow::mw, [this] {

        timer->stop();
        if (is_editable) {
            if (dont_save_result_to_de == false) {
                save_to_data_engine();
            }
            load_actual_value();
        }
        set_ui_visibility();
    });
}

void DataEngineInput::set_explanation_text(const std::string &extra_explanation) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    this->extra_explanation = QString().fromStdString(extra_explanation);
    label_extra_explanation->setText(this->extra_explanation);
}

bool DataEngineInput::get_is_editable() {
    return is_editable;
}

void DataEngineInput::set_visible(bool visible) {
    is_visible = visible;
    set_ui_visibility();
}

void DataEngineInput::set_enabled(bool enabled) {
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
void DataEngineInput::start_timer() {
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

void DataEngineInput::resizeMe(QResizeEvent *event) {
    total_width = event->size().width();
    set_ui_visibility();
}

void DataEngineInput::set_total_visibilty() {
    assert(MainWindow::gui_thread == QThread::currentThread());

    label_extra_explanation->setVisible(is_visible && is_enabled && is_waiting);
    label_de_description->setVisible(is_visible);
    label_de_desired_value->setVisible(is_visible);
    label_de_actual_value->setVisible(is_visible && (is_editable == false));
    label_ok->setVisible(is_visible && ((is_editable == false) || (is_enabled == false)));
    lineedit->setVisible(is_visible && is_editable && is_enabled);
    //    });
}

void DataEngineInput::scale_columns() {
    assert(MainWindow::gui_thread == QThread::currentThread());

    int col_size = 7;
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
        button_exceptional_approval->setFixedWidth((total_width / col_size));
    } else if (is_enabled && is_visible) {
        label_extra_explanation->setFixedWidth((total_width / col_size));
        //label_de_description->setFixedWidth((total_width / col_size));
        label_de_desired_value->setFixedWidth((total_width / col_size));
        label_de_actual_value->setFixedWidth((total_width / col_size));
        label_ok->setFixedWidth((total_width / col_size));
        button_next->setFixedWidth((total_width / col_size));
        button_exceptional_approval->setFixedWidth((total_width / col_size));

    } else if ((is_enabled == false) && is_visible) {
        label_extra_explanation->setFixedWidth((total_width / col_size));
        //label_de_description->setFixedWidth((total_width / col_size));
        label_de_desired_value->setFixedWidth((total_width / col_size));
        label_de_actual_value->setFixedWidth((total_width / col_size));
        label_ok->setFixedWidth((total_width / col_size));
        button_next->setFixedWidth((total_width / col_size));

        button_exceptional_approval->setFixedWidth((total_width / col_size));
    } else if (!is_visible) {
    }
}

void DataEngineInput::set_button_visibility(bool next, bool yes_no) {
    assert(MainWindow::gui_thread == QThread::currentThread());

    this->label_no->setVisible(yes_no);
    this->button_no->setVisible(yes_no);

    this->label_yes->setVisible(yes_no);
    this->button_yes->setVisible(yes_no);

    this->label_next->setVisible(next);
    this->button_next->setVisible(next);

    this->button_exceptional_approval->setVisible(next || yes_no);
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
