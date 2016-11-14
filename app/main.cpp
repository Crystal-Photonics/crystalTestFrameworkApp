#include "../src/mainwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
	QCoreApplication::setOrganizationName("CPG");
	QCoreApplication::setApplicationName("Crystal Test Framework App");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
