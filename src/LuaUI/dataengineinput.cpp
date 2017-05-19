#include "dataengineinput.h"

#include "Windows/mainwindow.h"
#include "config.h"
#include "global.h"
#include "ui_container.h"
#include "util.h"

#include <QCheckBox>
#include <QDoubleValidator>
#include <QEventLoop>
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
DataEngineInput::DataEngineInput(UI_container *parent, Data_engine *data_engine, std::string field_id, std::string extra_explanation,
                                 std::string empty_value_placeholder, std::string desired_prefix, std::string actual_prefix)
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
    , actual_prefix{QString::fromStdString(actual_prefix)} {
    hlayout = new QHBoxLayout;

    hlayout->addWidget(label_extra_explanation);
    hlayout->addWidget(label_de_description);
    hlayout->addWidget(label_de_desired_value);
    hlayout->addWidget(label_de_actual_value);
    hlayout->addWidget(label_ok);
    parent->add(hlayout);

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

    auto *vlayout_next = new QVBoxLayout;
    label_next = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence, "").toString() + ")", parent);
    button_next = new QPushButton("next", parent);
    vlayout_next->addWidget(button_next);
    vlayout_next->addWidget(label_next);
    hlayout->addLayout(vlayout_next);
    QKeySequence shortcut_yes = QKeySequence{QSettings{}.value(Globals::confirm_key_sequence, "").toString()};
    button_next->setShortcut(shortcut_yes);
    callback_next = QObject::connect(button_next, &QPushButton::clicked, [this]() {});

    start_timer();
    timer->start(500);
    parent->scroll_to_bottom();
}

DataEngineInput::~DataEngineInput() {
    timer->stop();
    QObject::disconnect(callback_timer);
    QObject::disconnect(callback_next);
    QObject::disconnect(callback_bool_false);
    QObject::disconnect(callback_bool_true);
    set_enabled(false);
}
///\endcond

void DataEngineInput::load_actual_value() {
    if (data_engine->value_complete(field_id)) {
        timer->stop();
        QObject::disconnect(callback_timer);

        QString val = data_engine->get_actual_value(field_id);
        label_de_actual_value->setText(this->actual_prefix + " " + val + " " + data_engine->get_unit(this->field_id));
        if (data_engine->value_in_range(field_id)) {
            label_ok->setText("OK");
        } else {
            label_ok->setText("fail");
        }
        if (!editable) {
            label_extra_explanation->setText(" ");
            if (button_next) {
                button_next->setVisible(false);
                button_next = nullptr;
            }
            if (label_next) {
                label_next->setVisible(false);
                label_next = nullptr;
            }
        }
    }
}

void DataEngineInput::set_editable() {
    if (editable) {
        return;
    }
    timer->stop();
    QObject::disconnect(callback_timer);
    label_extra_explanation->setText(extra_explanation);

    auto entry_type = data_engine->get_entry_type(field_id);
    editable = true;
    label_de_actual_value->setVisible(false);
    switch (entry_type) {
        case EntryType::Bool: {
            if (button_next) {
                button_next->setVisible(false);
                button_next = nullptr;
            }
            if (label_next) {
                label_next->setVisible(false);
                label_next = nullptr;
            }

            auto *vlayout_yes = new QVBoxLayout;
            auto *vlayout_no = new QVBoxLayout;
            label_yes = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence, "").toString() + ")", parent);
            label_no = new QLabel("(or " + QSettings{}.value(Globals::cancel_key_sequence, "").toString() + ")", parent);
            button_no = new QPushButton("false", parent);
            button_yes = new QPushButton("true", parent);

            vlayout_no->addWidget(button_no);
            vlayout_no->addWidget(label_no);

            vlayout_yes->addWidget(button_yes);
            vlayout_yes->addWidget(label_yes);

            hlayout->insertLayout(3, vlayout_no);
            hlayout->insertLayout(3, vlayout_yes);

            QKeySequence shortcut_yes = QKeySequence{QSettings{}.value(Globals::confirm_key_sequence, "").toString()};
            QKeySequence shortcut_no = QKeySequence{QSettings{}.value(Globals::cancel_key_sequence, "").toString()};
            button_yes->setShortcut(shortcut_yes);
            button_no->setShortcut(shortcut_no);

            callback_bool_false = QObject::connect(button_no, &QPushButton::clicked, [this]() { bool_result = false; });
            callback_bool_true = QObject::connect(button_yes, &QPushButton::clicked, [this]() { bool_result = true; });

        } break;
        case EntryType::Numeric: {
            lineedit = new QLineEdit(parent);
            lineedit->setText(empty_value_placeholder);
            hlayout->insertWidget(3, lineedit);
            auto validator = new QDoubleValidator(parent);
            lineedit->setValidator(validator);
        } break;
        case EntryType::String: {
            lineedit = new QLineEdit(parent);
            lineedit->setText(empty_value_placeholder);
            hlayout->insertWidget(3, lineedit);

        } break;
        default:
            break;
    }
}

void DataEngineInput::save_to_data_engine() {
    if (editable) {
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

void DataEngineInput::await_event() {
    QEventLoop event_loop;
    enum { confirm_pressed, skip_pressed, cancel_pressed };
    std::array<std::unique_ptr<QShortcut>, 3> shortcuts;
    MainWindow::mw->execute_in_gui_thread([&event_loop, &shortcuts] {
        const char *settings_keys[] = {Globals::confirm_key_sequence, Globals::skip_key_sequence, Globals::cancel_key_sequence};
        for (std::size_t i = 0; i < shortcuts.size(); i++) {
            shortcuts[i] = std::make_unique<QShortcut>(QKeySequence::fromString(QSettings{}.value(settings_keys[i], "").toString()), MainWindow::mw);
            QObject::connect(shortcuts[i].get(), &QShortcut::activated, [&event_loop, i] { event_loop.exit(i); });
        }
    });
    auto exit_value = event_loop.exec();
    Utility::promised_thread_call(MainWindow::mw, [&shortcuts] { std::fill(std::begin(shortcuts), std::end(shortcuts), nullptr); });
}

void DataEngineInput::set_visible(bool visible) {
    label_extra_explanation->setVisible(visible);
    label_de_description->setVisible(visible);
    label_de_desired_value->setVisible(visible);
    label_de_actual_value->setVisible(visible);
    label_ok->setVisible(visible);
    if (lineedit) {
        lineedit->setVisible(visible);
    }
    if (button_no) {
        button_no->setVisible(visible);
    }
    if (button_yes) {
        button_yes->setVisible(visible);
    }
    if (label_yes) {
        label_yes->setVisible(visible);
    }
    if (label_no) {
        label_no->setVisible(visible);
    }
    if (button_next) {
        button_next->setVisible(visible);
    }
    if (label_next) {
        label_next->setVisible(visible);
    }
}

void DataEngineInput::set_enabled(bool enabled) {
    label_extra_explanation->setEnabled(enabled);
    label_de_description->setEnabled(enabled);
    label_de_desired_value->setEnabled(enabled);
    label_de_actual_value->setEnabled(enabled);
    if (enabled == false) {
        label_extra_explanation->setText(" ");
    } else {
        label_extra_explanation->setText(extra_explanation);
    }
    label_ok->setEnabled(enabled);
    if (lineedit) {
        lineedit->setEnabled(enabled);
    }
    if (button_no) {
        button_no->setVisible(enabled);
    }
    if (button_yes) {
        button_yes->setVisible(enabled);
    }
    if (label_yes) {
        label_yes->setVisible(enabled);
    }
    if (label_no) {
        label_no->setVisible(enabled);
    }
    if (button_next) {
        button_next->setVisible(enabled);
    }
    if (label_next) {
        label_next->setVisible(enabled);
    }
}

void DataEngineInput::clear_explanation() {
    label_extra_explanation->setText(" ");
}
///\cond HIDDEN_SYMBOLS
void DataEngineInput::start_timer() {
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() {
        if (blink_state) {
            label_de_actual_value->setText(" ");
            label_extra_explanation->setText(" ");
        } else {
            label_de_actual_value->setText(actual_prefix + " " + empty_value_placeholder);
            label_extra_explanation->setText(extra_explanation);
        }
        blink_state = !blink_state;

    });
}
///\endcond
