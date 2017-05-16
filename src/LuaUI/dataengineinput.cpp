#include "dataengineinput.h"

#include "ui_container.h"

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
    : label_extra_explanation{new QLabel(parent)}
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
    QHBoxLayout *hlayout = new QHBoxLayout;

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
    label_de_desired_value->setText(this->desired_prefix+" " +data_engine->get_desired_value_as_string(this->field_id)+" "+data_engine->get_unit(this->field_id));
    label_de_desired_value->setWordWrap(true);
    label_de_actual_value->setText(this->actual_prefix+" " + this->empty_value_placeholder);
    label_de_actual_value->setWordWrap(true);
    label_ok->setText("-");
    start_timer();
    timer->start(500);
}

DataEngineInput::~DataEngineInput() {
    timer->stop();
    QObject::disconnect(callback_connection);
    label_extra_explanation->setEnabled(false);
    label_de_description->setEnabled(false);
    label_de_desired_value->setEnabled(false);
    label_de_actual_value->setEnabled(false);
    label_ok->setEnabled(false);
}
///\endcond

void DataEngineInput::load_actual_value() {
    if (data_engine->value_complete(field_id)) {
        timer->stop();
        QObject::disconnect(callback_connection);

        QString val = data_engine->get_actual_value(field_id);
        label_de_actual_value->setText(this->actual_prefix+" "+val+" "+data_engine->get_unit(this->field_id));
        if (data_engine->value_in_range(field_id)) {
            label_ok->setText("OK");
        } else {
            label_ok->setText("fail");
        }
        label_extra_explanation->setText(" ");
    }
}

void DataEngineInput::set_visible(bool visible) {
    label_extra_explanation->setVisible(visible);
    label_de_description->setVisible(visible);
    label_de_desired_value->setVisible(visible);
    label_de_actual_value->setVisible(visible);
    label_ok->setVisible(visible);
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
}

void DataEngineInput::clear_explanation() {
    label_extra_explanation->setText(" ");
}
///\cond HIDDEN_SYMBOLS
void DataEngineInput::start_timer() {
    callback_connection = QObject::connect(timer, &QTimer::timeout, [this]() {
#if 1
        if (blink_state) {
            label_de_actual_value->setText(" ");
            label_extra_explanation->setText(" ");
        } else {
            label_de_actual_value->setText(actual_prefix+" " +empty_value_placeholder);
            label_extra_explanation->setText(extra_explanation);
        }
        blink_state = !blink_state;
#endif
    });
}
///\endcond
