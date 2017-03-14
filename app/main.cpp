#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <cassert>

static QtMessageHandler old_handler;

void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	switch (type) {
		case QtCriticalMsg:
		case QtFatalMsg:
			QMessageBox::critical(MainWindow::mw, "Qt Error", '\"' + msg + '\"' + "\nwas caused by" + context.function + " in " + context.file + ":" +
																  QString::number(context.line) +
																  ".\nAdd a breakpoint in main.cpp:28 to inspect the stack.\n"
																  "Press CTRL+C to copy the content of this message box to your clipboard.");
			break;
		case QtWarningMsg:
			QMessageBox::warning(MainWindow::mw, "Qt Warning", '\"' + msg + '\"' + "\nwas caused by" + context.function + " in " + context.file + ":" +
																   QString::number(context.line) +
																   ".\nAdd a breakpoint in main.cpp:28 to inspect the stack.\n"
																   "Press CTRL+C to copy the content of this message box to your clipboard.");
			break;
		case QtDebugMsg:
		case QtInfoMsg:;
	}
	old_handler(type, context, msg);
}

int main(int argc, char *argv[]) {
	QCoreApplication::setOrganizationName("CPG");
	QCoreApplication::setApplicationName("Crystal Test Framework App");
	old_handler = qInstallMessageHandler(message_handler);
	QApplication a(argc, argv);
    MainWindow w;
    w.show();
	return a.exec();
}
