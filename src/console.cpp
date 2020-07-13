#include "console.h"
#include "Windows/mainwindow.h"
#include "util.h"

#include <QFileInfo>
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
    state.s << "<b><a href=\"" << plink.link << "\">";
    append_to(state.s, plink.text);
    state.s << "</a></b>";
    return std::move(*this);
}

Console_handle::ConsoleProxy::ConsoleProxy(QPlainTextEdit *console, QStringList s, QString prefix, QColor color, bool fat)
    : state{console, std::move(s), std::move(prefix), std::move(color), fat} {}

Console_handle::ConsoleProxy::ConsoleProxy(Console_handle::ConsoleProxyState state)
    : state{std::move(state)} {}

void print_lua_source_link(std::string src, std::string_view line, Console_handle::ConsoleProxy &&proxy) {
    const auto &luafiles = MainWindow::get_luafiles();
    auto source = std::string_view{src};
    if (source.size() > MainWindow::partial_luafile_path_size) {
        source.remove_prefix(source.size() - MainWindow::partial_luafile_path_size);
    }
    const auto luafile_it = luafiles.find(QString::fromStdString(std::string{source}));
    if (luafile_it == std::end(luafiles)) { //unknown file
        std::move(proxy) << "Unknown file <b>" + QString::fromStdString(src + ":" + std::string{line}) + "</b>";
        qDebug() << "Failed finding partial file:\n" << QString::fromStdString(std::string{source});
        for (const auto &prefix : luafiles) {
            qDebug() << prefix.first;
        }
    } else {                                 //known file
        if (luafile_it->second.size() > 1) { //multiple options
            std::move(proxy) << "Ambiguous file <b>" + QString::fromStdString(std::string{source} + ":" + std::string{line});
            int n = 1;
            for (auto &path : luafile_it->second) {
                std::move(proxy) << ' ' << Console_Link{path + ':' + QString::fromStdString(std::string{line}), "[" + QString::number(n++) + "]"};
            }
            std::move(proxy) << "</b>";
        } else { //found the exact file
            const auto filepath = luafile_it->second.front();
            std::move(proxy) << Console_Link{filepath + ':' + QString::fromStdString(std::string{line}),
                                             QString::fromStdString(src) + ':' + QString::fromStdString(std::string{line})};
        }
    }
}

Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(Sol_error_message sol_message) && {
    for (auto &lline : sol_message.message.split('\n')) {
        linkify_print(lline.toStdString());
        std::move(*this) << '\n';
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

void Console_handle::ConsoleProxy::linkify_print(std::string line) {
    static std::regex partial_filename_regex{R"((\.\.\.)?((?:(?!\.\.\.)[^:])*):(\d+): (.*))"};
    static std::regex string_source_regex{R"((.*)\[string \"([^\"]*)\"]:(\d+)(.*))"};
    const char *prefix = "";
    std::move(*this) << prefix;
    prefix = "\n";
    std::smatch match;
    if (std::regex_search(line, match, string_source_regex)) {
        if (match[1].matched) {
            std::move(*this) << QString::fromStdString(match[1]);
        }
        std::move(*this) << "<b>" << match[2] << ":" << match[3] << "</b>";
        if (match[4].matched) {
            linkify_print(match[4]);
        }
    } else if (std::regex_search(line, match, partial_filename_regex)) {
        const auto line_number = match[3].str();
        const auto post_message = match[4];
        const auto start = static_cast<std::size_t>(match.position());
        const auto pre_message = std::string_view{&*std::begin(line), start};
        std::move(*this) << pre_message;
        print_lua_source_link(match[2].str(), line_number, std::move(*this));
        std::move(*this) << ' ';
        linkify_print(post_message);
    } else {
        std::move(*this) << line;
    }
}

QPlainTextEdit *Console::get_plaintext_edit() const {
    return console;
}
