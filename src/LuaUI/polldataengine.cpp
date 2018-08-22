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
PollDataEngine::PollDataEngine(UI_container *parent_, ScriptEngine *script_engine, Data_engine *data_engine_, QStringList items)
    : UI_widget{parent_}
    , timer{new QTimer(parent_)}
    , data_engine{data_engine_}
    , script_engine{script_engine} {
    grid_layout = new QGridLayout;
    parent->add(grid_layout, this);
    assert(data_engine);
    for (auto field_id : items) {
        FieldEntry field_entry{};
        field_entry.field_id = field_id;
        const int row = field_entries.count();
        field_entry.label_de_description = new QLabel(parent_);
        field_entry.label_de_desired_value = new QLabel(parent_);
        field_entry.label_de_actual_value = new QLabel(parent_);
        field_entry.label_ok = new QLabel(parent_);

        grid_layout->addWidget(field_entry.label_de_description, row, 1, Qt::AlignTop);
        grid_layout->addWidget(field_entry.label_de_desired_value, row, 2, Qt::AlignTop);
        grid_layout->addWidget(field_entry.label_de_actual_value, row, 3, Qt::AlignTop);
        grid_layout->addWidget(field_entry.label_ok, row, 4, Qt::AlignTop);

        QString desc = data_engine->get_description(field_id);
        field_entry.label_de_description->setText(desc);
        field_entry.label_de_description->setWordWrap(true);
        QString s = data_engine->get_desired_value_as_string(field_id);
        field_entry.label_de_desired_value->setText(s);
        field_entry.label_de_desired_value->setWordWrap(true);
        field_entry.label_de_actual_value->setText(empty_value_placeholder);
        field_entry.label_de_actual_value->setWordWrap(true);
        field_entry.label_ok->setText(empty_value_placeholder);
        if (!data_engine->is_desired_value_set(field_id)) {
            QString s = data_engine->get_unit(field_id);
            if (s.size()) {
                field_entry.label_de_desired_value->setText(empty_value_placeholder + "[" + s + "]");
            } else {
                field_entry.label_de_desired_value->setText("");
            }
        }

        auto entry_type = data_engine->get_entry_type(field_id);
        switch (entry_type) {
            case EntryType::Bool: {
                field_entry.field_type = FieldEntry::FieldType::Bool;

            } break;
            case EntryType::Numeric: {
                field_entry.field_type = FieldEntry::FieldType::Numeric;
            } break;
            case EntryType::String: {
                field_entry.field_type = FieldEntry::FieldType::String;
            } break;
            default:
                assert(0);
                break;
        }
        field_entries.append(field_entry);
    }

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

void PollDataEngine::refresh() {
    Utility::promised_thread_call(MainWindow::mw, [this] {
        load_actual_value();
    });
}

void PollDataEngine::load_actual_value() {
    assert(MainWindow::gui_thread == QThread::currentThread());
    for (FieldEntry &field_entry : field_entries) {
        if (data_engine->value_complete(field_entry.field_id)) {
            QString val = data_engine->get_actual_value(field_entry.field_id);
            field_entry.label_de_actual_value->setText(val);
        } else {
            QString s = data_engine->get_unit(field_entry.field_id);
            if (s.size()) {
                field_entry.label_de_actual_value->setText(empty_value_placeholder + "[" + s + "]");
            } else {
                field_entry.label_de_actual_value->setText("");
            }
        }

        if (data_engine->value_in_range_in_instance(field_entry.field_id)) {
            field_entry.label_ok->setText("OK");
        } else {
            field_entry.label_ok->setText("Fail");
        }
    }
}

bool PollDataEngine::is_in_range() {
    for (FieldEntry &field_entry : field_entries) {
        if (!data_engine->value_in_range(field_entry.field_id)) {
            return false;
        }
    }
    return true;
}

void PollDataEngine::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    is_visible = visible;
    set_ui_visibility();
}

void PollDataEngine::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    is_enabled = enabled;

    set_ui_visibility();
    if (enabled) {
        timer->start(BLINK_INTERVAL_MS);
    } else {
        timer->stop();
    }
    refresh();
}

///\cond HIDDEN_SYMBOLS
void PollDataEngine::start_timer() {
#if 1
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() { load_actual_value(); });
#endif
}

void PollDataEngine::resizeMe(QResizeEvent *event) {
    (void)event;
    //    set_ui_visibility();
}

void PollDataEngine::set_total_visibilty() {
    assert(MainWindow::gui_thread == QThread::currentThread());
    for (FieldEntry &field_entry : field_entries) {
        field_entry.label_de_description->setVisible(is_visible);
        field_entry.label_de_desired_value->setVisible(is_visible);
        field_entry.label_de_actual_value->setVisible(is_visible);
        field_entry.label_ok->setVisible(is_visible);
    }
}

void PollDataEngine::set_labels_enabled() {
    assert(MainWindow::gui_thread == QThread::currentThread());
    for (FieldEntry &field_entry : field_entries) {
        field_entry.label_de_description->setEnabled(is_enabled);
        field_entry.label_de_desired_value->setEnabled(is_enabled);
        field_entry.label_de_actual_value->setEnabled(is_enabled);
        field_entry.label_ok->setEnabled(is_enabled);
    }
}

void PollDataEngine::set_ui_visibility() {
#if 1
    Utility::promised_thread_call(MainWindow::mw, [this] {

        if (init_ok == false) {
            return;
        }
        set_labels_enabled();
        set_total_visibilty();

    });
#endif
}

///\endcond
