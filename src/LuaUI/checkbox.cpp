#include "checkbox.h"

///\cond HIDDEN_SYMBOLS
CheckBox::CheckBox(QSplitter *parent) {
    checkbox = new QCheckBox(parent);
    parent->addWidget(checkbox);
}

CheckBox::~CheckBox() {
    checkbox->setEnabled(false);
}
///\endcond


void CheckBox::set_checked(bool const checked){
    return checkbox->setChecked(checked);
}


bool CheckBox::get_checked() const {
    return checkbox->isChecked();
}

std::string CheckBox::get_text() const {
    return checkbox->text().toStdString();
}

void CheckBox::set_text(std::string const text){
    return checkbox->setText(QString::fromStdString(text));
}
