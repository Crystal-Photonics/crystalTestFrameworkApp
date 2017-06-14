#include "hline.h"

#include "label.h"
#include "ui_container.h"

#include <QDebug>
#include <QFrame>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
HLine::HLine(UI_container *parent)
    : UI_widget{parent}
    , line{new QFrame(parent)} {
    QVBoxLayout *layout = new QVBoxLayout;

    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    layout->addWidget(line, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);

    parent->scroll_to_bottom();
}

HLine::~HLine() {
    line->setEnabled(false);
}

void HLine::set_visible(bool visible) {
    line->setVisible(visible);
}
