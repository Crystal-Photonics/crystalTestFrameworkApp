#include "qt_util.h"

#include <QTableWidget>

QWidget *Utility::replace_tab_widget(QTabWidget *tabs, int index, QWidget *new_widget, const QString &title) {
	const auto old_widget = tabs->widget(index);
	const auto old_index = tabs->currentIndex();
	tabs->removeTab(index);
	tabs->insertTab(index, new_widget, title);
	tabs->setCurrentIndex(old_index);
	return old_widget;
}
