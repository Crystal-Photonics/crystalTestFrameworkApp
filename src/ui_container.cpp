#include "ui_container.h"

#include <QVBoxLayout>
#include <QWidget>
#include <memory>

UI_container::UI_container(QWidget *parent)
	: QScrollArea{parent}
	, layout{new QVBoxLayout} {
	auto widget = std::make_unique<QWidget>();
	widget->setMinimumSize({100, 100});
	//widget->setFixedWidth(1000);
	widget->setLayout(layout);
	setWidget(widget.release());
}

void UI_container::add_below(QWidget *widget) {
	layout->addWidget(widget);
}
