#include "qt_util.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTableWidget>

QWidget *Utility::replace_tab_widget(QTabWidget *tabs, int index, QWidget *new_widget, const QString &title) {
	const auto old_widget = tabs->widget(index);
	const auto old_index = tabs->currentIndex();
	tabs->removeTab(index);
	tabs->insertTab(index, new_widget, title);
	tabs->setCurrentIndex(old_index);
	return old_widget;
}

Event_filter::Event_filter(QObject *parent)
	: QObject(parent) {}

Event_filter::Event_filter(QObject *parent, std::function<bool(QEvent *)> function)
	: QObject(parent)
	, callbacks({std::move(function)}) {}

void Event_filter::add_callback(std::function<bool(QEvent *)> function) {
	callbacks.emplace_back(std::move(function));
}

void Event_filter::clear() {
	callbacks.clear();
}

bool Event_filter::eventFilter(QObject *object, QEvent *ev) {
	bool retval = false;
	for (auto &function : callbacks) {
		retval |= function(ev);
	}
	return retval;
}
