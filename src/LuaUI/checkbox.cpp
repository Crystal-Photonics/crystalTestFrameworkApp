#include "checkbox.h"
#include "Windows/mainwindow.h"
#include "ui_container.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
CheckBox::CheckBox(UI_container *parent, const std::string text)
    : UI_widget{parent}
    , checkbox(new QCheckBox(parent)) {
    QVBoxLayout *layout = new QVBoxLayout;
    QLabel *label = new QLabel(parent);
    label->setText(" ");
    layout->addWidget(label);
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI

    layout->addWidget(checkbox, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);

    checkbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

    set_text(text);
    parent->scroll_to_bottom();
}

CheckBox::~CheckBox() {
    checkbox->setEnabled(false);
}
///\endcond

void CheckBox::set_checked(bool const checked) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return checkbox->setChecked(checked);
}

bool CheckBox::get_checked() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return checkbox->isChecked();
}

std::string CheckBox::get_text() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return checkbox->text().toStdString();
}

void CheckBox::set_text(std::string const text) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return checkbox->setText(QString::fromStdString(text));
}

void CheckBox::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    checkbox->setVisible(visible);
}
