#ifndef CONSOLE_H
#define CONSOLE_H

#include "channel_codec_wrapper.h"

#include <QColor>
#include <QStringList>
#include <QTextEdit>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

struct Console {
	private:
	struct ConsoleProxy;

	public:
	static QTextEdit *console;
	static ConsoleProxy warning(QTextEdit *console = nullptr);
	static ConsoleProxy note(QTextEdit *console = nullptr);
	static ConsoleProxy error(QTextEdit *console = nullptr);
	static ConsoleProxy debug(QTextEdit *console = nullptr);

	private:
	struct ConsoleProxy {
		template <class T>
		ConsoleProxy &&operator<<(const T &t) &&;
		~ConsoleProxy();
		QTextEdit *console;
		QStringList s;
		QString prefix;
		QColor color;
	};
	static std::false_type qstringlist_appandable(...);
	template <class T>
	static auto qstringlist_appandable(T &&t) -> decltype((QStringList{} << t, std::true_type()));
	template <class T>
	static auto append_to(QStringList &s, T &&t) -> std::enable_if_t<decltype(qstringlist_appandable(t))::value> {
		s << t;
	}
	template <class T>
	static auto append_to(QStringList &s, T &&t) -> std::enable_if_t<decltype(qstringlist_appandable(t))::value == false> {
		std::stringstream ss;
		ss << t;
		s << ss.str().c_str();
	}
};

template <class T>
Console::ConsoleProxy &&Console::ConsoleProxy::operator<<(const T &t) && {
	append_to(s, t);
	return std::move(*this); //TODO: I'm not sure if this is correct or we return an expired temporary
}

#endif // CONSOLE_H
