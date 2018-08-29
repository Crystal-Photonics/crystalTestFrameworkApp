#include "userwaitlabel.h"

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
#include <QMovie>
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
UserWaitLabel::UserWaitLabel(UI_container *parent, ScriptEngine *script_engine, std::string extra_explanation)
    : UI_widget{parent}
    , label_user_instruction{new QLabel(parent)}
    , timer{new QTimer(parent)}
    , instruction_text{QString::fromStdString(extra_explanation)}
    , script_engine(script_engine)

{
    hlayout = new QHBoxLayout;

    hlayout->addWidget(label_user_instruction, 0, Qt::AlignTop);

    spinner_label = new QLabel(parent);
    QMovie *movie = new QMovie("://src/LuaUI/ajax-loader.gif");
    spinner_label->setMovie(movie); //":/MyPreciousRes/MyResources.pro"
    movie->start();

    hlayout->addWidget(spinner_label, 0, Qt::AlignTop);

    parent->add(hlayout, this);

    label_user_instruction->setText(QString::fromStdString(extra_explanation));
    label_user_instruction->setWordWrap(true);

    start_timer();
    timer->start(500);
    auto sp_w = QSizePolicy::Maximum;
    auto sp_h = QSizePolicy::MinimumExpanding;

    label_user_instruction->setSizePolicy(sp_w, sp_h);

    set_enabled(true);
    parent->scroll_to_bottom();
    is_init = true;
    scale_columns();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

UserWaitLabel::~UserWaitLabel() {
    assert(MainWindow::gui_thread == QThread::currentThread());
    //Utility::promised_thread_call(MainWindow::mw, [this] {
    //MainWindow::mw->execute_in_gui_thread(script_engine, [this] {   //
    timer->stop();
    QObject::disconnect(callback_timer);

    set_enabled(false);
    // });
}
///\endcond

void UserWaitLabel::scale_columns() {
    int col_size = 6;

    label_user_instruction->setFixedWidth(5 * total_width / col_size);
}

bool UserWaitLabel::run_hotkey_loop() {
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

void UserWaitLabel::set_text(const std::string &instruction_text) {
    this->instruction_text = QString().fromStdString(instruction_text);
    label_user_instruction->setText(this->instruction_text);
}

void UserWaitLabel::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI

    label_user_instruction->setEnabled(enabled);
    label_user_instruction->setText(instruction_text);

    spinner_label->setVisible(enabled);

    if (enabled) {
        timer->start(500);

    } else {
        timer->stop();
    }
}

void UserWaitLabel::resizeMe(QResizeEvent *event) {
    total_width = event->size().width();
    if (is_init) {
        scale_columns();
    }
}

///\cond HIDDEN_SYMBOLS
void UserWaitLabel::start_timer() {
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
