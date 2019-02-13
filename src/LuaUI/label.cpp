#include "label.h"
#include "Windows/mainwindow.h"
#include "ui_container.h"
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
Label::Label(UI_container *parent, const std::string text)
    : UI_widget{parent}
    , label{new QLabel(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);

    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    set_text(text);
    normal_font_size = label->font().pointSize();
    label->setWordWrap(true);
    parent->scroll_to_bottom();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

Label::~Label() {
    label->setEnabled(false);
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}
///\endcond

std::string Label::get_text() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return label->text().toStdString();
}

void Label::set_text(std::string const text) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return label->setText(QString::fromStdString(text));
}

void Label::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    label->setVisible(visible);
}

void Label::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    label->setEnabled(enabled);
}

void Label::set_font_size(bool big_font) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    auto font = label->font();
    if (big_font) {
        font.setPointSize(normal_font_size * 4);
    } else {
        font.setPointSize(normal_font_size);
    }
    font.setBold(big_font);
    label->setFont(font);
}
