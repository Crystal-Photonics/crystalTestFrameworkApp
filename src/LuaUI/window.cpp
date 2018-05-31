#include "window.h"
#include "Windows/mainwindow.h"
#include "testrunner.h"
#include "ui_container.h"

#include <QApplication>
#include <QCloseEvent>
#include <QSplitter>
#include <QVBoxLayout>

///\cond HIDDEN_SYMBOLS
Window::Window(TestRunner *test)
	: QWidget(MainWindow::mw, Qt::Window)
	, test(test) {
	auto layout = new QVBoxLayout(this);
	layout->addWidget(test->get_lua_ui_container());
	setLayout(layout);
	setWindowTitle(test->get_name());
	test->get_lua_ui_container()->show();
	show();
}

QSize Window::sizeHint() const {
	return test->get_lua_ui_container()->sizeHint();
}

void Window::closeEvent(QCloseEvent *event) {
	if (test->is_running()) {
		if (QMessageBox::question(this, tr(""), tr("Selected script %1 is still running. Abort it now?").arg(test->get_name()),
								  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
			test->interrupt();
			test->join();
		} else {
			event->ignore();
			return; //canceled closing the window
		}
	}
	QApplication::processEvents();
	MainWindow::mw->remove_test_runner(test);
	QApplication::processEvents();
	event->accept();
}
///\endcond
