#include "spinbox.h"
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
    : label{new QLabel(parent)}
    , spinbox{new QSpinBox(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;
    label->setVisible(false);
    layout->addWidget(label);
    layout->addWidget(spinbox, 1, Qt::AlignBottom);
    parent->add(layout);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    spinbox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    spinbox->setMaximum(100);
}

SpinBox::~SpinBox() {
    spinbox->setReadOnly(true);
}
///\endcond

int SpinBox::get_value() const {
    return spinbox->value();
}

void SpinBox::set_max_value(const int value)
{
    spinbox->setMaximum(value);
    if (spinbox->value() > value){
        spinbox->setValue(value);
    }
}

void SpinBox::set_min_value(const int value)
{
    spinbox->setMinimum(value);
    if (spinbox->value() < value){
        spinbox->setValue(value);
    }
}

void SpinBox::set_value(const int value) {
    spinbox->setValue(value);
}



void SpinBox::set_caption(const std::string &caption) {
    label->setText(QString::fromStdString(caption));
    label->setVisible(label->text().size());
}

std::string SpinBox::get_caption() const {
    return label->text().toStdString();
}

void SpinBox::set_visible(bool visible)
{
    spinbox->setVisible(visible);
    label->setVisible(visible);
}

