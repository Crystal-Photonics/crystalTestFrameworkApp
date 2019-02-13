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
    layout->addWidget(spinbox, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    spinbox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
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
