#include "userinstructionlabel.h"

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
UserInstructionLabel::UserInstructionLabel(UI_container *parent, std::string extra_explanation)
    : parent{parent}
    , label_user_instruction{new QLabel(parent)}
    , timer{new QTimer(parent)}
    , instruction_text{QString::fromStdString(extra_explanation)}

{
    hlayout = new QHBoxLayout;

    hlayout->addWidget(label_user_instruction);

    parent->add(hlayout);

    label_user_instruction->setText(QString::fromStdString(extra_explanation));
    label_user_instruction->setWordWrap(true);

    auto *vlayout_next = new QVBoxLayout;
    label_next = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence, "").toString() + ")", parent);
    button_next = new QPushButton("next", parent);

    auto *vlayout_yes = new QVBoxLayout;
    label_yes = new QLabel("(or " + QSettings{}.value(Globals::confirm_key_sequence, "").toString() + ")", parent);
    button_yes = new QPushButton("yes", parent);

    auto *vlayout_no = new QVBoxLayout;
    label_no = new QLabel("(or " + QSettings{}.value(Globals::cancel_key_sequence, "").toString() + ")", parent);
    button_no = new QPushButton("no", parent);

    vlayout_next->addWidget(button_next);
    vlayout_next->addWidget(label_next);

    vlayout_yes->addWidget(button_yes);
    vlayout_yes->addWidget(label_yes);

    vlayout_no->addWidget(button_no);
    vlayout_no->addWidget(label_no);

    hlayout->addLayout(vlayout_next);
    hlayout->addLayout(vlayout_yes);
    hlayout->addLayout(vlayout_no);

#if 0
    QKeySequence shortcut_yes = QKeySequence{QSettings{}.value(Globals::confirm_key_sequence, "").toString()};
    button_yes->setShortcut(shortcut_yes);

    QKeySequence shortcut_no = QKeySequence{QSettings{}.value(Globals::cancel_key_sequence, "").toString()};
    button_no->setShortcut(shortcut_no);

    button_next->setShortcut(shortcut_yes);
#endif
    callback_button_yes = QObject::connect(button_yes, &QPushButton::clicked, [this]() {this->event_loop.exit(confirm_pressed);});
    callback_button_no = QObject::connect(button_no, &QPushButton::clicked, [this]() {this->event_loop.exit(cancel_pressed);});
    callback_button_next = QObject::connect(button_next, &QPushButton::clicked, [this]() {this->event_loop.exit(confirm_pressed);});

    start_timer();
    timer->start(500);
    set_enabled(true);
    parent->scroll_to_bottom();
}

UserInstructionLabel::~UserInstructionLabel() {
    event_loop.exit(cancel_pressed);
    timer->stop();
    QObject::disconnect(callback_timer);
    QObject::disconnect(callback_button_next);
    QObject::disconnect(callback_button_no);
    QObject::disconnect(callback_button_yes);
    set_enabled(false);
}
///\endcond

void UserInstructionLabel::await_event() {
    is_question_mode = false;
    run_hotkey_loop();
}

bool UserInstructionLabel::await_yes_no() {

    is_question_mode = true;
    return run_hotkey_loop();

}

bool UserInstructionLabel::run_hotkey_loop()
{
    set_enabled(true);
    std::array<std::unique_ptr<QShortcut>, 3> shortcuts;
    MainWindow::mw->execute_in_gui_thread([this, &shortcuts] {
        const char *settings_keys[] = {Globals::confirm_key_sequence, Globals::skip_key_sequence, Globals::cancel_key_sequence};
        for (std::size_t i = 0; i < shortcuts.size(); i++) {
            shortcuts[i] = std::make_unique<QShortcut>(QKeySequence::fromString(QSettings{}.value(settings_keys[i], "").toString()), MainWindow::mw);
            QObject::connect(shortcuts[i].get(), &QShortcut::activated, [this, i] { this->event_loop.exit(i); });
        }
    });
    assert(!event_loop.isRunning());
    auto exit_value = event_loop.exec();
    Utility::promised_thread_call(MainWindow::mw, [&shortcuts] { std::fill(std::begin(shortcuts), std::end(shortcuts), nullptr); });
    if (exit_value == confirm_pressed) {
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
    label_user_instruction->setEnabled(enabled);
    label_user_instruction->setText(instruction_text);

    button_yes->setVisible(enabled && is_question_mode);
    label_yes->setVisible(enabled && is_question_mode);

    button_no->setVisible(enabled && is_question_mode);
    label_no->setVisible(enabled && is_question_mode);

    button_next->setVisible(enabled && !is_question_mode);
    label_next->setVisible(enabled && !is_question_mode);

    if (enabled) {
        timer->start();
    } else {
        timer->stop();
    }
}

///\cond HIDDEN_SYMBOLS
void UserInstructionLabel::start_timer() {
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() {
        if (blink_state) {
            label_user_instruction->setText(" ");
        } else {
            label_user_instruction->setText(instruction_text);
        }
        blink_state = !blink_state;

    });
}
///\endcond
