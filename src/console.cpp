#include "console.h"
#include "Windows/mainwindow.h"
#include "util.h"

#include <QPlainTextEdit>
#include <QTime>

QPlainTextEdit *Console_handle::console = nullptr;
MainWindow *Console_handle::mw = nullptr;

Console_handle::ConsoleProxy Console_handle::warning(QPlainTextEdit *console) {
    if (console) {
        if (console->parent()) {
            console->setVisible(true);
        }
    }
	return {console ? console : Console_handle::console, QStringList{}, "Warning", QColor("orangered")};
}

Console_handle::ConsoleProxy Console_handle::note(QPlainTextEdit *console) {
	return {console ? console : Console_handle::console, QStringList{}, "Note", Qt::black};
}

Console_handle::ConsoleProxy Console_handle::error(QPlainTextEdit *console) {
    if (console) {
        if (console->parent()) {
            //qDebug() << "Console::error parent:" << ;
            console->setVisible(true);
        }
    }
	return {console ? console : Console_handle::console, QStringList{}, "Error", Qt::darkRed};
}

Console_handle::ConsoleProxy Console_handle::debug(QPlainTextEdit *console) {
	return {console ? console : Console_handle::console, QStringList{}, "Debug", Qt::darkGreen};
}

Console_handle::ConsoleProxy Console_handle::script(QPlainTextEdit *console) {
    if (console) {
        if (console->parent()) {
            //qDebug() << "Console::error parent:" << ;
            console->setVisible(true);
        }
    }
	return {console, {}, "Script", Qt::black, {}, true};
}

Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(Console_Link plink) {
	link = std::move(plink.link);
	return std::move(*this);
}

Console_handle::ConsoleProxy::~ConsoleProxy() {
    if (s.isEmpty()) {
        return;
    }
    QString s_br = Utility::to_human_readable_binary_data(s.join(" "));
	s_br = s_br.replace("\n", "<br>");
    if (fat) {
        mw->append_html_to_console("<font color=\"#" + QString::number(color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) + ": " +
									   prefix + ": </plaintext><b><plaintext>" + s_br + "</plaintext></b></font>\n",
                                   console);
    } else {
        mw->append_html_to_console("<font color=\"#" + QString::number(color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) + ": " +
                                       prefix + ": " + s_br + "</plaintext></font>\n",
                                   console);
    }
}

QPlainTextEdit *Console::get_plaintext_edit() const {
	return console;
}
