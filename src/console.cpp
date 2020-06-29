#include "console.h"
#include "Windows/mainwindow.h"
#include "util.h"

#include <QPlainTextEdit>
#include <QTime>
#include <regex>
#include <tuple>
#include <utility>

QPlainTextEdit *Console_handle::console = nullptr;

Console_handle::ConsoleProxy Console_handle::warning(QPlainTextEdit *console) {
    return {console ? console : Console_handle::console, QStringList{}, "Warning", QColor("orangered")};
}

Console_handle::ConsoleProxy Console_handle::note(QPlainTextEdit *console) {
    return {console ? console : Console_handle::console, QStringList{}, "Note", Qt::black};
}

Console_handle::ConsoleProxy Console_handle::error(QPlainTextEdit *console) {
    return {console ? console : Console_handle::console, QStringList{}, "Error", Qt::darkRed};
}

Console_handle::ConsoleProxy Console_handle::debug(QPlainTextEdit *console) {
    return {console ? console : Console_handle::console, QStringList{}, "Debug", Qt::darkGreen};
}

Console_handle::ConsoleProxy Console_handle::script(QPlainTextEdit *console) {
    return {console, {}, "Script", Qt::black, true};
}

Console_handle::ConsoleProxy Console_handle::from_state(Console_handle::ConsoleProxyState state) {
    return {std::move(state)};
}

Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(Console_Link plink) && {
    state.s << "<a href=\"" << plink.link << "\">";
    append_to(state.s, plink.text);
    state.s << "</a>";
    return std::move(*this);
}

Console_handle::ConsoleProxy::ConsoleProxy(QPlainTextEdit *console, QStringList s, QString prefix, QColor color, bool fat)
    : state{console, std::move(s), std::move(prefix), std::move(color), fat} {}

Console_handle::ConsoleProxy::ConsoleProxy(Console_handle::ConsoleProxyState state)
    : state{std::move(state)} {}

Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(Sol_error_message sol_message) && {
    const char *prefix = "";
    //return std::move(*this) << "In " << sol_message.script_name << ": " << sol_message.message;
    std::regex regex{R"(\.\.\.)?((?:(?!\.\.\.)[^:])*):(\d+): (.*)"};
    for (auto &lline : sol_message.message.split('\n')) {
        const auto line = lline.toStdString();
        std::move(*this) << prefix;
        prefix = "\n";
        std::smatch match;
        if (std::regex_search(line, match, regex)) {
            const auto line_number = QString::fromStdString(match[3].str());
            const auto post_message = match[4];
            const auto start = static_cast<std::size_t>(match.position());
            const auto pre_message = std::string_view{&*std::begin(line), start};
            std::move(*this) << pre_message << Console_Link{QString::fromStdString(match[2].str()), sol_message.script_path + ':' + line_number}
                             << sol_message.script_name + ':' + line_number << ": " << post_message;
        } else {
            std::move(*this) << line;
        }
    }
    return std::move(*this);
}

Console_handle::ConsoleProxy::ConsoleProxy(Console_handle::ConsoleProxy &&other)
    : state{std::move(other.state)} {
    other.state.console = nullptr;
}

Console_handle::ConsoleProxy &Console_handle::ConsoleProxy::operator=(Console_handle::ConsoleProxy &&other) {
    std::swap(state, other.state);
    other.state.console = nullptr;
    return *this;
}

Console_handle::ConsoleProxy::~ConsoleProxy() {
    if (state.s.isEmpty()) {
        return;
    }
    QString s_br = Utility::to_human_readable_binary_data(state.s.join(""));
    s_br = s_br.replace("\n", "<br>");
    auto text = state.fat ? "<font color=\"#" + QString::number(state.color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) + ": " +
                                state.prefix + ": </plaintext><b><plaintext>" + s_br + "</plaintext></b></font>\n" :
                            "<font color=\"#" + QString::number(state.color.rgb(), 16) + "\"><plaintext>" + QTime::currentTime().toString(Qt::ISODate) + ": " +
                                state.prefix + ": " + s_br + "</plaintext></font>\n";
    Utility::thread_call(MainWindow::mw, [text = std::move(text), console = this->state.console] {
        if (console->parent()) {
            console->setVisible(true);
        }
        MainWindow::mw->append_html_to_console(text, console);
    });
}

QPlainTextEdit *Console::get_plaintext_edit() const {
    return console;
}
