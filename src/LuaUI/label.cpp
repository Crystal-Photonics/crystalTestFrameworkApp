#include "label.h"


///\cond HIDDEN_SYMBOLS
Label::Label(QSplitter *parent, const std::string text) {
    label = new QLabel(parent);
    parent->addWidget(label);
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
