#include "label.h"
#include "ui_container.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
Label::Label(UI_container *parent, const std::string text)
	: label{new QLabel(parent)} {
	parent->add(label);
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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
