#include "ui_container.h"
#include "config.h"

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSettings>
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
	set_actions();
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

void UI_container::set_actions() {
	struct {
		const char *settings_key;
		const char *default_settings_key;
		void (UI_container::*signal)();
	} actions_data[] = {
		{Globals::confirm_key_sequence_key, Globals::default_confirm_key_sequence_key, &UI_container::confirm_pressed},
		{Globals::skip_key_sequence_key, Globals::default_skip_key_sequence_key, &UI_container::skip_pressed},
		{Globals::cancel_key_sequence_key, Globals::default_cancel_key_sequence_key, &UI_container::cancel_pressed},
	};
	for (auto &action_data : actions_data) {
		const auto index = &action_data - actions_data;
		auto &action = shortcut_actions[index];
		action = std::make_unique<QAction>(this);
		action->setShortcut(QKeySequence::fromString(QSettings{}.value(action_data.settings_key, action_data.default_settings_key).toString()));
		connect(action.get(), &QAction::triggered, this, action_data.signal);
		addAction(action.get());
	}
}

UI_widget::UI_widget(UI_container *parent_)
    : parent{parent_} {}

UI_widget::~UI_widget() {
    parent->remove_me_from_resize_list(this);
}
