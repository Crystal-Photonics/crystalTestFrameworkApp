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
            const auto file = context.file ? context.file : "unknown file";
            const auto function = context.function ? context.function : "unknown function";
            qDebug() << msg << "in" << file << ":" << function << ':' << context.line;
            if (context.file == nullptr) {
                asm("int $3");
                break;
            }
            auto show_message = [msg, file = std::string{file}, function = std::string{function}, line = context.line] {
                QMessageBox::critical(MainWindow::mw, "Qt Error",
                                      '\"' + msg + '\"' + "\nwas caused by " + function.c_str() + " in " + file.c_str() + ":" + QString::number(line) +
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
            const auto file = context.file ? context.file : "unknown file";
            const auto function = context.function ? context.function : "unknown function";
            qDebug() << msg << "in" << file << ":" << function << ':' << context.line;
            if (context.file == nullptr) {
                asm("int $3");
                break;
            }
            auto show_message = [msg, file = std::string{file}, function = std::string{function}, line = context.line] {
                QMessageBox::warning(MainWindow::mw, "Qt Warning",
                                     '\"' + msg + '\"' + "\nwas caused by " + function.c_str() + " in " + file.c_str() + ":" + QString::number(line) +
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
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();
    //  a.setWindowIcon(QIcon("://src/icons/app_icon_multisize.png"));
    auto retval = a.exec();
    w.shutdown();
    return retval;
}
