#include "console.h"
#include "Windows/mainwindow.h"
#include "util.h"

#include <QPlainTextEdit>
#include <QTime>

QPlainTextEdit *Console::console = nullptr;
MainWindow *Console::mw = nullptr;
bool Console::use_human_readable_encoding = true;

Console::ConsoleProxy Console::warning(QPlainTextEdit *console) {
    if (console) {
        if (console->parent()) {
            console->setVisible(true);
        }
    }
    return {console ? console : Console::console, QStringList{}, "Warning", Qt::darkYellow};
}

Console::ConsoleProxy Console::warning(std::unique_ptr<QPlainTextEdit> &console) {
    return warning(console.get());
}

Console::ConsoleProxy Console::note(QPlainTextEdit *console) {
    return {console ? console : Console::console, QStringList{}, "Note", Qt::black};
}

Console::ConsoleProxy Console::note(std::unique_ptr<QPlainTextEdit> &console) {
    return note(console.get());
}

Console::ConsoleProxy Console::error(QPlainTextEdit *console) {
    if (console) {
        if (console->parent()) {
            //qDebug() << "Console::error parent:" << ;
            console->setVisible(true);
        }
    }
    return {console ? console : Console::console, QStringList{}, "Error", Qt::darkRed};
}

Console::ConsoleProxy Console::error(std::unique_ptr<QPlainTextEdit> &console) {
    return error(console.get());
}

Console::ConsoleProxy Console::debug(QPlainTextEdit *console) {
    return {console ? console : Console::console, QStringList{}, "Debug", Qt::darkGreen};
}

Console::ConsoleProxy Console::debug(std::unique_ptr<QPlainTextEdit> &console) {
    return debug(console.get());
}

Console::ConsoleProxy Console::script(QPlainTextEdit *console) {
    if (console) {
        if (console->parent()) {
            //qDebug() << "Console::error parent:" << ;
            console->setVisible(true);
        }
    }
    return {console, {}, "Script", Qt::black, true};
}

Console::ConsoleProxy::~ConsoleProxy() {
    if (s.isEmpty()) {
        return;
    }
    if (fat) {
        mw->append_html_to_console("<font color=\"#" + QString::number(color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) + ": " +
                                       prefix + ": </plaintext><b><plaintext>" + Utility::to_human_readable_binary_data(s.join(" ")) +
                                       "</plaintext></b></font>\n",
                                   console);
    } else {
        mw->append_html_to_console("<font color=\"#" + QString::number(color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) + ": " +
                                       prefix + ": " + Utility::to_human_readable_binary_data(s.join(" ")) + "</plaintext></font>\n",
                                   console);
    }
}
