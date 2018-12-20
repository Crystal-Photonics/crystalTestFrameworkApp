#include "Windows/mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <cassert>

static QtMessageHandler old_handler;

void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    switch (type) {
        case QtCriticalMsg:
        case QtFatalMsg: {
            qDebug() << msg;
            auto show_message = [ msg, file = QString{context.file}, function = QString{context.function}, line = context.line ] {
                QMessageBox::critical(MainWindow::mw, "Qt Error", '\"' + msg + '\"' + "\nwas caused by " + function + " in " + file + ":" +
                                                                      QString::number(line) + ".\nAdd a breakpoint in main.cpp:" + QString::number(line - 2) +
                                                                      " to inspect the stack.\n"
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
            auto show_message = [ msg, file = QString{context.file}, function = QString{context.function}, line = context.line ] {
                QMessageBox::warning(MainWindow::mw, "Qt Warning", '\"' + msg + '\"' + "\nwas caused by " + function + " in " + file + ":" +
                                                                       QString::number(line) + ".\nAdd a breakpoint in main.cpp:" + QString::number(line - 2) +
                                                                       " to inspect the stack.\n"
                                                                       "Press CTRL+C to copy the content of this message box to your clipboard.");
            };
            if (MainWindow::mw != nullptr) {
                MainWindow::mw->execute_in_gui_thread(show_message);
            } else {
                show_message();
            }
        }

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
    w.showMaximized();
    //  a.setWindowIcon(QIcon("://src/icons/app_icon_multisize.png"));
    auto retval = a.exec();
    w.shutdown();
    return retval;
}
