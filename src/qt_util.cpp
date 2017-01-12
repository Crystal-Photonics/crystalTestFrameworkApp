#include "qt_util.h"

#include <QTableWidget>

QWidget *Utility::replace_tab_widget(QTabWidget *tabs, int index, QWidget *new_widget, const QString &title) {
	auto old_widget = tabs->widget(index);
	tabs->removeTab(index);
	if (new_widget) {
		tabs->insertTab(index, new_widget, title);
	}
	return old_widget;
}
