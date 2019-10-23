#include "spinbox.h"
#include "Windows/mainwindow.h"
#include "ui_container.h"
#include <QInputDialog>
#include <QLabel>
#include <QSpinBox>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
SpinBox::SpinBox(UI_container *parent)
    : UI_widget{parent}
    , label{new QLabel(parent)}
    , spinbox{new QSpinBox(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;
    label->setText(" ");
    layout->addWidget(label);
	layout->addWidget(spinbox);
    parent->add(layout, this);
    spinbox->setMaximum(100);
    parent->scroll_to_bottom();
    assert(MainWindow::gui_thread == QThread::currentThread());
}

SpinBox::~SpinBox() {
    spinbox->setReadOnly(true);
    assert(MainWindow::gui_thread == QThread::currentThread());
}
///\endcond

int SpinBox::get_value() const {
    assert(MainWindow::gui_thread == QThread::currentThread());
    return spinbox->value();
}

void SpinBox::set_max_value(const int value) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    spinbox->setMaximum(value);
    if (spinbox->value() > value) {
        spinbox->setValue(value);
    }
}

void SpinBox::set_min_value(const int value) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    spinbox->setMinimum(value);
    if (spinbox->value() < value) {
        spinbox->setValue(value);
    }
}

void SpinBox::set_value(const int value) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    if (value > spinbox->maximum()) {
        throw std::runtime_error(
            QObject::tr("spinbox: trying to set value to %1 but its maximum value is %2").arg(value).arg(spinbox->maximum()).toStdString());
    }
    if (value < spinbox->minimum()) {
        throw std::runtime_error(
            QObject::tr("spinbox: trying to set value to %1 but its minimum value is %2").arg(value).arg(spinbox->minimum()).toStdString());
    }
    spinbox->setValue(value);
}

void SpinBox::set_caption(const std::string &caption) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    label->setText(QString::fromStdString(caption));
    if (caption == "") {
        label->setText(" ");
    }
    //label->setVisible(label->text().size());
}

std::string SpinBox::get_caption() const {
    assert(MainWindow::gui_thread == QThread::currentThread());
    return label->text().toStdString();
}

void SpinBox::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread());
    spinbox->setVisible(visible);
    label->setVisible(visible);
}
