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
    QString text;
};

//passing this to a ConsoleProxy causes the path to be replaced and linked
struct Sol_error_message {
    QString message;
    QString script_path; //the path for the editor to find the file
    QString script_name; //the displayed path
};

struct Console_handle {
    struct ConsoleProxyState {
        QPlainTextEdit *console;
        QStringList s;
        QString prefix;
        QColor color;
        bool fat = false;
    };

    private:
    struct ConsoleProxy {
        ConsoleProxy(QPlainTextEdit *console, QStringList s, QString prefix, QColor color, bool fat = false);
        ConsoleProxy(ConsoleProxyState state);
        template <class T>
        ConsoleProxy &&operator<<(const T &t) &&;
        ConsoleProxy &&operator<<(Console_Link link) &&;
        ConsoleProxy &&operator<<(Sol_error_message sol_message) &&;
        ConsoleProxy(ConsoleProxy &&other);
        ConsoleProxy &operator=(ConsoleProxy &&other);
        ~ConsoleProxy();

        ConsoleProxyState state;
    };

    public:
    static QPlainTextEdit *console;
    static ConsoleProxy warning(QPlainTextEdit *console = nullptr);
    static ConsoleProxy note(QPlainTextEdit *console = nullptr);
    static ConsoleProxy error(QPlainTextEdit *console = nullptr);
    static ConsoleProxy debug(QPlainTextEdit *console = nullptr);
    static ConsoleProxy script(QPlainTextEdit *console);
    static ConsoleProxy from_state(ConsoleProxyState state);

    private:
    static std::false_type qstringlist_appandable(...);
    template <class T>
    static auto qstringlist_appandable(T &&t) -> decltype((QStringList{} << t, std::true_type()));
    template <class T>
    static void append_to(QStringList &s, T &&t) {
        if constexpr (decltype(qstringlist_appandable(t))::value) {
            s << t;
        } else {
            std::stringstream ss;
            ss << t;
            s << QString::fromStdString(ss.str());
        }
    }
};

template <class T>
Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(const T &t) && {
    append_to(state.s, t);
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

    static auto from_state(Console_handle::ConsoleProxyState state) {
        return Console_handle::from_state(std::move(state));
    }

    QPlainTextEdit *get_plaintext_edit() const;

    private:
    QPlainTextEdit *console;
};

#endif // CONSOLE_H
