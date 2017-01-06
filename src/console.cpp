#include "console.h"
#include "mainwindow.h"
#include "util.h"

#include <QTime>

QPlainTextEdit *Console::console = nullptr;
MainWindow *Console::mw = nullptr;

Console::ConsoleProxy Console::warning(QPlainTextEdit *console) {
	return {console ? console : Console::console, {}, "Warning", Qt::darkYellow};
}

Console::ConsoleProxy Console::note(QPlainTextEdit *console) {
	return {console ? console : Console::console, {}, "Note", Qt::black};
}

Console::ConsoleProxy Console::error(QPlainTextEdit *console) {
	return {console ? console : Console::console, {}, "Error", Qt::darkRed};
}

Console::ConsoleProxy Console::debug(QPlainTextEdit *console) {
	return {console ? console : Console::console, {}, "Debug", Qt::darkGreen};
}

Console::ConsoleProxy::~ConsoleProxy() {
	if (s.isEmpty()) {
		return;
	}
	emit mw->append_html_to_console("<font color=\"#" + QString::number(color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) +
										": " + prefix + ": " + Utility::to_human_readable_binary_data(s.join(" ")) + "</plaintext></font>\n",
									console);
}
