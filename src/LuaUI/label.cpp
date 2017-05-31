#include "label.h"
#include "ui_container.h"

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
Label::Label(UI_container *parent, const std::string text)
    : label{new QLabel(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout,this);

    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    set_text(text);
    normal_font_size = label->font().pointSize();
    label->setWordWrap(true);
    parent->scroll_to_bottom();
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

void Label::set_visible(bool visible) {
    label->setVisible(visible);
}

void Label::set_font_size(bool big_font) {
    auto font = label->font();
    if (big_font) {
        font.setPointSize(normal_font_size * 4);
    } else {
        font.setPointSize(normal_font_size);
    }
    font.setBold(big_font);
    label->setFont(font);
}
