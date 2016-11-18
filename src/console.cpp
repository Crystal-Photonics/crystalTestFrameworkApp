#include "console.h"

#include <QTime>

QTextEdit *Console::console = nullptr;

Console::ConsoleProxy Console::warning() {
	return {console, {}, "Warning"};
}

Console::ConsoleProxy Console::note()
{
	return {console, {}, "Note"};
}

Console::ConsoleProxy::~ConsoleProxy() {
	if (s.isEmpty()) {
		return;
	}
	console->append(QTime::currentTime().toString(Qt::ISODate) + ": " + prefix + ": " + s.join(" "));
}
