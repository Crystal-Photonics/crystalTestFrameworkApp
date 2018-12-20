#include "userinstructionlabel.h"

#include "Windows/mainwindow.h"
#include "config.h"
#include "ui_container.h"
#include "util.h"

#include <QResizeEvent>
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
UserInstructionLabel::UserInstructionLabel(UI_container *parent, ScriptEngine *script_engine, std::string extra_explanation)
    : UI_widget{parent}
    , label_user_instruction{new QLabel(parent)}
    , timer{new QTimer(parent)}
    , instruction_text{QString::fromStdString(extra_explanation)}
    , script_engine(script_engine)

{
    hlayout = new QHBoxLayout;

    hlayout->addWidget(label_user_instruction, 0, Qt::AlignTop);

    parent->add(hlayout, this);

    label_user_instruction->setText(QString::fromStdString(extra_explanation));
    label_user_instruction->setWordWrap(true);

    auto *vlayout_next = new QVBoxLayout;
    label_next = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence_key, "").toString() + ")", parent);
    button_next = new QPushButton("next", parent);

    auto *vlayout_yes = new QVBoxLayout;
    label_yes = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence_key, "").toString() + ")", parent);
    button_yes = new QPushButton("yes", parent);

    auto *vlayout_no = new QVBoxLayout;
    label_no = new QLabel("(or " + QSettings{}.value(Globals::cancel_key_sequence_key, "").toString() + ")", parent);
    button_no = new QPushButton("no", parent);

    vlayout_next->addWidget(button_next);
    vlayout_next->addWidget(label_next);
    vlayout_next->addStretch();

    vlayout_yes->addWidget(button_yes);
    vlayout_yes->addWidget(label_yes);
    vlayout_yes->addStretch();

    vlayout_no->addWidget(button_no);
    vlayout_no->addWidget(label_no);
    vlayout_no->addStretch();

    hlayout->addLayout(vlayout_next);
    hlayout->addLayout(vlayout_yes);
    hlayout->addLayout(vlayout_no);

    callback_button_yes = QObject::connect(button_yes, &QPushButton::clicked,
                                           [script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent::confirm_pressed); });
    callback_button_no = QObject::connect(button_no, &QPushButton::clicked,
                                          [script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent::cancel_pressed); });
    callback_button_next = QObject::connect(button_next, &QPushButton::clicked,
                                            [script_engine]() { script_engine->hotkey_event_queue_send_event(HotKeyEvent::HotKeyEvent::confirm_pressed); });

    start_timer();
    timer->start(500);
    auto sp_w = QSizePolicy::Maximum;
    auto sp_h = QSizePolicy::MinimumExpanding;
    button_next->setSizePolicy(sp_w, sp_h);
    button_yes->setSizePolicy(sp_w, sp_h);
    button_no->setSizePolicy(sp_w, sp_h);
    label_user_instruction->setSizePolicy(sp_w, sp_h);

    set_enabled(true);
    parent->scroll_to_bottom();
    is_init = true;
    scale_columns();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

UserInstructionLabel::~UserInstructionLabel() {
    assert(MainWindow::gui_thread == QThread::currentThread());
    //Utility::promised_thread_call(MainWindow::mw, [this] {
    //MainWindow::mw->execute_in_gui_thread(script_engine, [this] {   //
    timer->stop();
    QObject::disconnect(callback_timer);
    QObject::disconnect(callback_button_next);
    QObject::disconnect(callback_button_no);
    QObject::disconnect(callback_button_yes);
    set_enabled(false);
    // });
}
///\endcond

void UserInstructionLabel::scale_columns() {
    int col_size = 6;
    button_next->setFixedWidth(total_width / col_size);
    button_no->setFixedWidth(total_width / (2 * col_size));
    button_yes->setFixedWidth(total_width / (2 * col_size));
    label_user_instruction->setFixedWidth(5 * total_width / col_size);
}

void UserInstructionLabel::await_event() {
    assert(MainWindow::gui_thread != QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    is_question_mode = false;
    run_hotkey_loop();
}

bool UserInstructionLabel::await_yes_no() {
    is_question_mode = true;
    return run_hotkey_loop();
}

bool UserInstructionLabel::run_hotkey_loop() {
    assert(MainWindow::gui_thread != QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    Utility::promised_thread_call(MainWindow::mw, [this] {      //
        set_enabled(true);
    });
    if (script_engine->hotkey_event_queue_run() == HotKeyEvent::HotKeyEvent::confirm_pressed) {
        return true;
    } else {
        return false;
    }
}

void UserInstructionLabel::set_instruction_text(const std::string &instruction_text) {
    this->instruction_text = QString().fromStdString(instruction_text);
    label_user_instruction->setText(this->instruction_text);
}

void UserInstructionLabel::set_visible(bool visible) {
    label_user_instruction->setVisible(visible);

    button_yes->setVisible(visible && is_question_mode);
    label_yes->setVisible(visible && is_question_mode);

    button_no->setVisible(visible && is_question_mode);
    label_no->setVisible(visible && is_question_mode);

    button_next->setVisible(visible && !is_question_mode);
    label_next->setVisible(visible && !is_question_mode);
}

void UserInstructionLabel::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI

    label_user_instruction->setEnabled(enabled);
    label_user_instruction->setText(instruction_text);

    button_yes->setVisible(enabled && is_question_mode);
    label_yes->setVisible(enabled && is_question_mode);

    button_no->setVisible(enabled && is_question_mode);
    label_no->setVisible(enabled && is_question_mode);

    button_next->setVisible(enabled && !is_question_mode);
    label_next->setVisible(enabled && !is_question_mode);

    if (enabled) {
        timer->start(500);
    } else {
        timer->stop();
    }
}

void UserInstructionLabel::resizeMe(QResizeEvent *event) {
    total_width = event->size().width();
    if (is_init) {
        scale_columns();
    }
}

///\cond HIDDEN_SYMBOLS
void UserInstructionLabel::start_timer() {
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() {
        if (blink_state == Globals::ui_blink_ratio) {
            label_user_instruction->setText(" ");
            blink_state = 0;
        } else {
            label_user_instruction->setMinimumHeight(0);
            label_user_instruction->setMaximumHeight(16777215);
            label_user_instruction->setText(instruction_text);
            label_user_instruction->setFixedHeight(label_user_instruction->height());
        }
        blink_state++;

    });
}
///\endcond
