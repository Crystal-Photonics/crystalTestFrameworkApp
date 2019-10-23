///\cond HIDDEN_SYMBOLS
#include "hline.h"

#include "label.h"
#include "ui_container.h"

#include "Windows/mainwindow.h"
#include <QDebug>
#include <QFrame>
#include <QVBoxLayout>
#include <QWidget>


HLine::HLine(UI_container *parent)
    : UI_widget{parent}
    , line{new QFrame(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;

    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    layout->addWidget(line, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);

    parent->scroll_to_bottom();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

HLine::~HLine() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    line->setEnabled(false);
}

void HLine::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    line->setVisible(visible);
}
///\endcond
