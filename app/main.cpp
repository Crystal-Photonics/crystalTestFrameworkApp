#include "../src/mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <cassert>

static QtMessageHandler old_handler;

void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	switch (type) {
		case QtWarningMsg:
		case QtCriticalMsg:
		case QtFatalMsg:
			//assert(false);
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
