#include "checkbox.h"
#include "ui_container.h"

#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
CheckBox::CheckBox(UI_container *parent, const std::string text)
	: checkbox(new QCheckBox(parent)) {
	checkbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	parent->add(checkbox);
    set_text(text);
}

CheckBox::~CheckBox() {
    checkbox->setEnabled(false);
}
///\endcond

void CheckBox::set_checked(bool const checked) {
    return checkbox->setChecked(checked);
}

bool CheckBox::get_checked() const {
    return checkbox->isChecked();
}

std::string CheckBox::get_text() const {
    return checkbox->text().toStdString();
}

void CheckBox::set_text(std::string const text) {
    return checkbox->setText(QString::fromStdString(text));
}

void CheckBox::set_visible(bool visible)
{
    checkbox->setVisible(visible);
}
