#include "label.h"
#include <QVBoxLayout>

///\cond HIDDEN_SYMBOLS
Label::Label(QSplitter *parent, const std::string text) {

    base_widget = new QWidget(parent);
    label = new QLabel(base_widget);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(label);
    base_widget->setLayout(layout);
    parent->addWidget(base_widget);

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
