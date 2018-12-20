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

struct Console {
	private:
	struct ConsoleProxy;

	public:
	static QPlainTextEdit *console;
	static ConsoleProxy warning(QPlainTextEdit *console = nullptr);
	static ConsoleProxy warning(std::unique_ptr<QPlainTextEdit> &console);
	static ConsoleProxy note(QPlainTextEdit *console = nullptr);
	static ConsoleProxy note(std::unique_ptr<QPlainTextEdit> &console);
	static ConsoleProxy error(QPlainTextEdit *console = nullptr);
	static ConsoleProxy error(std::unique_ptr<QPlainTextEdit> &console);
	static ConsoleProxy debug(QPlainTextEdit *console = nullptr);
	static ConsoleProxy debug(std::unique_ptr<QPlainTextEdit> &console);
	static ConsoleProxy script(QPlainTextEdit *console);
	static MainWindow *mw;

	private:
	struct ConsoleProxy {
		template <class T>
		ConsoleProxy &&operator<<(const T &t) &&;
		~ConsoleProxy();
		QPlainTextEdit *console;
		QStringList s;
		QString prefix;
		QColor color;
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
Console::ConsoleProxy &&Console::ConsoleProxy::operator<<(const T &t) && {
	append_to(s, t);
	return std::move(*this); //TODO: I'm not sure if this is correct or we return an expired temporary
}

#endif // CONSOLE_H
