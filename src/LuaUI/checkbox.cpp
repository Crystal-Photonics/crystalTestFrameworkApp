#include "checkbox.h"
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

void CheckBox::set_visible(bool visible) {
    checkbox->setVisible(visible);
}
