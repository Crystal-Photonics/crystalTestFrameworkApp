#ifndef CONSOLE_H
#define CONSOLE_H

#include <QColor>
#include <QStringList>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

class MainWindow;
class QPlainTextEdit;

struct Console_Link {
	QString link;
};

struct Console_handle {
	private:
	struct ConsoleProxy;

	public:
	static QPlainTextEdit *console;
	static ConsoleProxy warning(QPlainTextEdit *console = nullptr);
	static ConsoleProxy note(QPlainTextEdit *console = nullptr);
	static ConsoleProxy error(QPlainTextEdit *console = nullptr);
	static ConsoleProxy debug(QPlainTextEdit *console = nullptr);
	static ConsoleProxy script(QPlainTextEdit *console);
	static MainWindow *mw;

	private:
	struct ConsoleProxy {
		template <class T>
		ConsoleProxy &&operator<<(const T &t) &&;
		ConsoleProxy &&operator<<(Console_Link link);
		~ConsoleProxy();
		QPlainTextEdit *console;
		QStringList s;
		QString prefix;
		QColor color;
		QString link;
		bool fat = false;
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
Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(const T &t) && {
	if (not link.isEmpty()) {
		s << "<a href=\"" << link << "\">";
	}
	append_to(s, t);
	if (not link.isEmpty()) {
		s << "</a>";
		link.clear();
	}
	return std::move(*this);
}

struct Console {
	Console(QPlainTextEdit *console)
		: console{console} {}
	auto warning() {
		return Console_handle::warning(console);
	}
	auto note() {
		return Console_handle::note(console);
	}
	auto error() {
		return Console_handle::error(console);
	}
	auto debug() {
		return Console_handle::debug(console);
	}

	QPlainTextEdit *get_plaintext_edit() const;

	private:
	QPlainTextEdit *console;
};

#endif // CONSOLE_H
