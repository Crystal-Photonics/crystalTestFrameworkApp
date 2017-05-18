#include "ui_container.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QWidget>
#include <algorithm>
#include <memory>

struct Widget_paragraph {
    Widget_paragraph(QVBoxLayout *parent)
        : layout{new QHBoxLayout} {
        parent->addLayout(layout);
    }

    void add(QWidget *widget) {
        layout->addWidget(widget, 1, Qt::AlignBottom);
    }

    void add(QLayout *layout) {
        this->layout->addLayout(layout);
    }

    QHBoxLayout *layout{};
};

UI_container::UI_container(QWidget *parent)
    : QScrollArea{parent}
    , layout{new QVBoxLayout} {
    auto widget = std::make_unique<QWidget>();
    widget->setMinimumSize({100, 100});
    widget->setLayout(layout);
    setWidget(widget.release());
}

UI_container::~UI_container() {
    //this destructor is required because we destruct forward declared objects Widget_paragraph
}

void UI_container::add(QWidget *widget) {
    if (paragraphs.size() < 2 || paragraphs.back().layout->count() >= column_count) {
        paragraphs.emplace_back(this->layout);
    }
    paragraphs.back().add(widget);
    trigger_resize();
}

void UI_container::add(QLayout *layout) {
    if (paragraphs.size() < 2 || paragraphs.back().layout->count() >= column_count) {
        paragraphs.emplace_back(this->layout);
    }
    paragraphs.back().add(layout);
    trigger_resize();
}

void UI_container::set_column_count(int columns) {
    paragraphs.emplace_back(this->layout);
    column_count = columns;
}

void UI_container::scroll_to_bottom() {
    //ensureVisible(0, height(), 0, 0);
    verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());
}

void UI_container::resizeEvent(QResizeEvent *event) {
    widget()->setFixedWidth(event->size().width());
    widget()->setFixedHeight(std::max(compute_size(event->size().width()), event->size().height()));
    QScrollArea::resizeEvent(event);
}

int UI_container::compute_size(int width) {
    return std::accumulate(std::begin(paragraphs), std::end(paragraphs), 0, [width](int size, const Widget_paragraph &paragraph) {
        if (paragraph.layout->hasHeightForWidth()) {
            return size + paragraph.layout->heightForWidth(width);
        }
        return size + paragraph.layout->sizeHint().height();
    });
}

void UI_container::trigger_resize() {
    const auto size = this->widget()->size();
    QResizeEvent resize_event{size, size};
    resizeEvent(&resize_event);
}
