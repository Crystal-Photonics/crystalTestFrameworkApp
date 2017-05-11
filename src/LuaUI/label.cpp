#include "label.h"
#include "ui_container.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

///\cond HIDDEN_SYMBOLS
Label::Label(UI_container *parent, const std::string text)
	: label{new QLabel(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout);

	label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    set_text(text);
}

Label::~Label() {
    label->setEnabled(false);
}
///\endcond

std::string Label::get_text() const {
    return label->text().toStdString();
}

void Label::set_text(std::string const text) {
    return label->setText(QString::fromStdString(text));
}

void Label::set_visible(bool visible)
{
    label->setVisible(visible);
}
