#include "label.h"
#include "ui_container.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

///\cond HIDDEN_SYMBOLS
Label::Label(UI_container *parent, const std::string text) {

    base_widget = new QWidget(parent);
    label = new QLabel(base_widget);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(label);
    base_widget->setLayout(layout);
	parent->add(base_widget);

    base_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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

void Label::set_text(std::string const text){
    return label->setText(QString::fromStdString(text));
}
