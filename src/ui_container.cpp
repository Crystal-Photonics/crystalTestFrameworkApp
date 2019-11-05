#include "ui_container.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <memory>

struct Widget_paragraph {
    Widget_paragraph(QVBoxLayout *parent)
        : layout{new QHBoxLayout}
        , lua_ui_widgets{} {
        parent->addLayout(layout);
    }

    void add(QWidget *widget, UI_widget *lua_ui_widget) {
		layout->addWidget(widget);
		if (lua_ui_widget) {
			lua_ui_widgets.push_back(lua_ui_widget);
		}
    }

    void add(QLayout *layout, UI_widget *lua_ui_widget) {
        this->layout->addLayout(layout);
		if (lua_ui_widget) {
			lua_ui_widgets.push_back(lua_ui_widget);
		}
	}

	void resizeEvent(QResizeEvent *event) {
		for (auto w : lua_ui_widgets) {
			if (w) {
				w->resizeMe(event);
			}
		}
	}

    void remove_me_from_resize_list(UI_widget *me) {
        for (auto &w : lua_ui_widgets) {
            if (w == me) {
                // qDebug() << "removed" << w;
                w = nullptr;
            }
        }
    }

    QHBoxLayout *layout{};

    std::vector<UI_widget *> lua_ui_widgets{};

    private:
};

UI_container::UI_container(QWidget *parent)
    : QScrollArea{parent}
    , layout{new QVBoxLayout} {
    auto widget = std::make_unique<QWidget>();
	widget->setMinimumSize({100, 100});
    widget->setLayout(layout);
	layout->setAlignment(Qt::AlignmentFlag::AlignTop);
    setWidget(widget.release());
	setWidgetResizable(true);
	const auto vscrollbar = verticalScrollBar();
	connect(vscrollbar, &QAbstractSlider::rangeChanged, [vscrollbar](int, int) { vscrollbar->setSliderPosition(vscrollbar->maximum()); });
}

UI_container::~UI_container() {
    //this destructor is required because we destruct forward declared objects Widget_paragraph
}

void UI_container::add(QWidget *widget, UI_widget *lua_ui_widget) {
    if (paragraphs.size() < 2 || paragraphs.back().layout->count() >= column_count) {
        paragraphs.emplace_back(this->layout);
    }
    paragraphs.back().add(widget, lua_ui_widget);
	QResizeEvent resize_event{size(), {0, 0}};
	resizeEvent(&resize_event);
}

void UI_container::add(QLayout *layout, UI_widget *lua_ui_widget) {
    if (paragraphs.size() < 2 || paragraphs.back().layout->count() >= column_count) {
        paragraphs.emplace_back(this->layout);
    }
    paragraphs.back().add(layout, lua_ui_widget);
	QResizeEvent resize_event{size(), {0, 0}};
	resizeEvent(&resize_event);
}

void UI_container::set_column_count(int columns) {
    paragraphs.emplace_back(this->layout);
    column_count = columns;
}

void UI_container::scroll_to_bottom() {
    //ensureVisible(0, height(), 0, 0);
    verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());
}

void UI_container::remove_me_from_resize_list(UI_widget *me) {
    for (auto &p : paragraphs) {
        p.remove_me_from_resize_list(me);
    }
}

void UI_container::resizeEvent(QResizeEvent *event) {
	QScrollArea::resizeEvent(event);
	for (auto &p : paragraphs) {
		p.resizeEvent(event);
	}
	scroll_to_bottom();
}

UI_widget::UI_widget(UI_container *parent_)
    : parent{parent_} {}

UI_widget::~UI_widget() {
    parent->remove_me_from_resize_list(this);
}
