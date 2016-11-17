#include "console.h"

#include <QTime>

QTextEdit *Console::console = nullptr;

Console::ConsoleProxy Console::warning() {
	return {console, {}};
}

Console::ConsoleProxy::~ConsoleProxy() {
	if (s.isEmpty()) {
		return;
	}
	console->append(QTime::currentTime().toString(Qt::ISODate) + ": " + s.join(" "));
}
