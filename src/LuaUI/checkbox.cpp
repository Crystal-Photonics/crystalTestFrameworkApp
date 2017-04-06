#include "checkbox.h"
#include "ui_container.h"

#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
CheckBox::CheckBox(UI_container *parent) {

    base_widget = new QWidget(parent);
    checkbox = new QCheckBox(base_widget);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(checkbox);
    base_widget->setLayout(layout);
	parent->add(base_widget);

    base_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    checkbox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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
