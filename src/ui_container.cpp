#include "ui_container.h"

#include <QDebug>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <memory>

UI_container::UI_container(QWidget *parent)
	: QScrollArea{parent}
	, layout{new QVBoxLayout} {
	auto widget = std::make_unique<QWidget>();
	widget->setMinimumSize({100, 100});
	widget->setLayout(layout);
	setWidget(widget.release());
}

void UI_container::add_below(QWidget *widget) {
	layout->addWidget(widget);
	widgets.push_back(widget);
	const auto size = this->widget()->size();
	QResizeEvent resize_event{size, size};
	resizeEvent(&resize_event);
}

void UI_container::resizeEvent(QResizeEvent *event) {
	widget()->setFixedWidth(event->size().width());
	widget()->setFixedHeight(std::max(compute_size(event->size().width()), event->size().height()));
	QScrollArea::resizeEvent(event);
}

int UI_container::compute_size(int width) {
	return std::accumulate(std::begin(widgets), std::end(widgets), 0, [width](int size, QWidget *widget) {
		if (widget->hasHeightForWidth()) {
			return size + widget->heightForWidth(width);
		}
		return size + widget->sizeHint().height();
	});
}
