#include "window.h"
#include "Windows/mainwindow.h"
#include "testrunner.h"
#include "ui_container.h"

#include <QApplication>
#include <QCloseEvent>
#include <QSplitter>
#include <QVBoxLayout>

///\cond HIDDEN_SYMBOLS
Window::Window(TestRunner *test, QString title)
	: QWidget(MainWindow::mw, Qt::Window)
	, test(test) {
	auto layout = new QVBoxLayout(this);
	layout->addWidget(test->get_lua_ui_container());
	setLayout(layout);
	setWindowTitle(title);
	test->get_lua_ui_container()->show();
	show();
}

QSize Window::sizeHint() const {
	return test->get_lua_ui_container()->sizeHint();
}

void Window::closeEvent(QCloseEvent *event) {
	MainWindow::mw->adopt_testrunner(test, windowTitle());
	event->accept();
}
///\endcond
