#include "dataengineinput.h"

#include "ui_container.h"

#include <QCheckBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QString>
#include <QTimer>
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
    start_timer();
    timer->start(500);
}

DataEngineInput::~DataEngineInput() {
    timer->stop();
    QObject::disconnect(callback_connection);
    set_enabled(false);
}
///\endcond

void DataEngineInput::load_actual_value() {
    if (data_engine->value_complete(field_id)) {
        timer->stop();
        QObject::disconnect(callback_connection);

        QString val = data_engine->get_actual_value(field_id);
        label_de_actual_value->setText(this->actual_prefix + " " + val + " " + data_engine->get_unit(this->field_id));
        if (data_engine->value_in_range(field_id)) {
            label_ok->setText("OK");
        } else {
            label_ok->setText("fail");
        }
        if (!editable) {
            label_extra_explanation->setText(" ");
        }
    }
}

void DataEngineInput::set_editable() {
    if (editable) {
        return;
    }
    timer->stop();
    QObject::disconnect(callback_connection);
    label_extra_explanation->setText(extra_explanation);

    auto entry_type = data_engine->get_entry_type(field_id);
    editable = true;
    label_de_actual_value->setVisible(false);
    switch (entry_type) {
        case EntryType::Bool: {
            checkbox = new QCheckBox(parent);
            checkbox->setText(actual_prefix);
            hlayout->insertWidget(3, checkbox);
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
                data_engine->set_actual_bool(field_id, checkbox->isChecked());

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

void DataEngineInput::set_visible(bool visible) {
    label_extra_explanation->setVisible(visible);
    label_de_description->setVisible(visible);
    label_de_desired_value->setVisible(visible);
    label_de_actual_value->setVisible(visible);
    label_ok->setVisible(visible);
    if (lineedit) {
        lineedit->setVisible(visible);
    }
    if (checkbox) {
        checkbox->setVisible(visible);
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
    if (checkbox) {
        checkbox->setEnabled(enabled);
    }
}

void DataEngineInput::clear_explanation() {
    label_extra_explanation->setText(" ");
}
///\cond HIDDEN_SYMBOLS
void DataEngineInput::start_timer() {
    callback_connection = QObject::connect(timer, &QTimer::timeout, [this]() {
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
