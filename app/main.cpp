#include "Windows/mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QSettings>
#include <QTranslator>
#include <cassert>

static QtMessageHandler old_handler;

static void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    switch (type) {
        case QtCriticalMsg:
        case QtFatalMsg: {
            qDebug() << msg;
            auto show_message = [msg, file = QString{context.file}, function = QString{context.function}, line = context.line] {
                QMessageBox::critical(MainWindow::mw, "Qt Error",
                                      '\"' + msg + '\"' + "\nwas caused by " + function + " in " + file + ":" + QString::number(line) +
                                          ".\n"
                                          "Press CTRL+C to copy the content of this message box to your clipboard.");
            };
            if (MainWindow::mw != nullptr) {
                MainWindow::mw->execute_in_gui_thread(show_message);
            } else {
                show_message();
            }
        } break;
        case QtWarningMsg: {
            qDebug() << msg;
            auto show_message = [msg, file = QString{context.file}, function = QString{context.function}, line = context.line] {
                QMessageBox::warning(MainWindow::mw, "Qt Warning",
                                     '\"' + msg + '\"' + "\nwas caused by " + function + " in " + file + ":" + QString::number(line) +
                                         ".\n"
                                         "Press CTRL+C to copy the content of this message box to your clipboard.");
            };
            if (MainWindow::mw != nullptr) {
                MainWindow::mw->execute_in_gui_thread(show_message);
            } else {
                show_message();
            }
        } break;
        case QtDebugMsg:
        case QtInfoMsg:;
    }
    old_handler(type, context, msg);
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("CPG");
    QCoreApplication::setApplicationName("Crystal Test Framework App");
    old_handler = qInstallMessageHandler(message_handler);
    //QSettings::setDefaultFormat(QSettings::IniFormat); //this way we would save the settings in an ini file. but better keep it in registry to maintain compatibility
    QApplication a(argc, argv);

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale::system(), "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        a.installTranslator(&qtTranslator);
    }

    MainWindow w;
    //  a.setWindowIcon(QIcon("://src/icons/app_icon_multisize.png"));
    auto retval = a.exec();
    w.shutdown();
    return retval;
}
