#include "console.h"

#include <QTime>

QTextEdit *Console::console = nullptr;

Console::ConsoleProxy Console::warning(QTextEdit *console) {
	return {console ? console : Console::console, {}, "Warning", Qt::darkYellow};
}

Console::ConsoleProxy Console::note(QTextEdit *console)
{
	return {console ? console : Console::console, {}, "Note", Qt::black};
}

Console::ConsoleProxy Console::error(QTextEdit *console)
{
	return {console ? console : Console::console, {}, "Error", Qt::darkRed};
}

Console::ConsoleProxy Console::debug(QTextEdit *console)
{
	return {console ? console : Console::console, {}, "Debug", Qt::darkGreen};
}

Console::ConsoleProxy::~ConsoleProxy() {
	if (s.isEmpty()) {
		return;
	}
	console->setTextColor(color);
	console->append(QTime::currentTime().toString(Qt::ISODate) + ": " + prefix + ": " + s.join(" "));
}
