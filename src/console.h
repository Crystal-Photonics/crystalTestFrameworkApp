#ifndef CONSOLE_H
#define CONSOLE_H

#include <QStringList>
#include <QTextEdit>

struct Console {
	private:
	struct ConsoleProxy;

	public:
	static QTextEdit *console;
	static ConsoleProxy warning();
	static ConsoleProxy note();

	private:
	struct ConsoleProxy {
		template <class T>
		ConsoleProxy &&operator<<(T &&t) &&{
			s << t;
			return std::move(*this);
			//QStringList tmp = std::move(s);
			//s.clear();
			//return {console, std::move(tmp)};
		}
		~ConsoleProxy();
		QTextEdit *console;
		QStringList s;
		QString prefix;
	};
};

#endif // CONSOLE_H