#include "progressbar.h"



#include "ui_container.h"

#include <QLabel>
#include <QProgressBar>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
ProgressBar::ProgressBar(UI_container *parent)
    : label{new QLabel(parent)}
    , progressbar{new QProgressBar(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;
    label->setText(" ");
    layout->addWidget(label);
    layout->addWidget(progressbar, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout,this);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    progressbar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    progressbar->setMaximum(100);
    parent->scroll_to_bottom();
}

ProgressBar::~ProgressBar() {

}
///\endcond


void ProgressBar::set_max_value(const int value) {
    progressbar->setMaximum(value);
    if (progressbar->value() > value) {
        progressbar->setValue(value);
    }
}

void ProgressBar::set_min_value(const int value) {
    progressbar->setMinimum(value);
    if (progressbar->value() < value) {
        progressbar->setValue(value);
    }
}

void ProgressBar::set_value(const int value) {
    progressbar->setValue(value);
}

void ProgressBar::increment_value()
{
    progressbar->setValue(progressbar->value()+1);
}

void ProgressBar::set_caption(const std::string &caption) {
    label->setText(QString::fromStdString(caption));
    if (caption == "") {
        label->setText(" ");
    }
    //label->setVisible(label->text().size());
}

std::string ProgressBar::get_caption() const {
    return label->text().toStdString();
}

void ProgressBar::set_visible(bool visible) {
    progressbar->setVisible(visible);
    label->setVisible(visible);
}
