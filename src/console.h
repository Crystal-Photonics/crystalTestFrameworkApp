#ifndef CONSOLE_H
#define CONSOLE_H

#include <QColor>
#include <QStringList>
#include <QTextEdit>
#include <string>

struct Console {
	private:
	struct ConsoleProxy;

	public:
	static QTextEdit *console;
	static ConsoleProxy warning(QTextEdit *console = nullptr);
	static ConsoleProxy note(QTextEdit *console = nullptr);
	static ConsoleProxy error(QTextEdit *console = nullptr);

	private:
	struct ConsoleProxy {
		template <class T>
		ConsoleProxy &&operator<<(const T &t) && {
			s << t;
			return std::move(*this); //TODO: I'm not sure if this is correct or we return an expired temporary
		}
		ConsoleProxy &&operator<<(const std::string &t) && {
			s << t.c_str();
			return std::move(*this); //TODO: I'm not sure if this is correct or we return an expired temporary
		}
		~ConsoleProxy();
		QTextEdit *console;
		QStringList s;
		QString prefix;
		QColor color;
	};
};

#endif // CONSOLE_H
