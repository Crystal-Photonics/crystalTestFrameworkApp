#include "console.h"
#include "Windows/mainwindow.h"
#include "util.h"

#include <QPlainTextEdit>
#include <QTime>
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
	state.link = std::move(plink.link);
	return std::move(*this);
}

Console_handle::ConsoleProxy::ConsoleProxy(QPlainTextEdit *console, QStringList s, QString prefix, QColor color, bool fat)
	: state{console, std::move(s), std::move(prefix), std::move(color), {}, fat} {}

Console_handle::ConsoleProxy::ConsoleProxy(Console_handle::ConsoleProxyState state)
	: state{std::move(state)} {}

Console_handle::ConsoleProxy &&Console_handle::ConsoleProxy::operator<<(Sol_error_message sol_message) && {
	QRegExp regex{R"(:(\d+):)"};
	bool first = true;
	for (auto &line : sol_message.message.split('\n')) {
		if (not first) {
			std::move(*this) << '\n';
		} else {
			first = false;
		}
		int match_index = regex.indexIn(line);
		if (match_index != -1) {
			auto line_number = regex.cap(1);
			auto post_message = line.mid(match_index + regex.matchedLength()).trimmed();
			QString premessage;
			int last_colon_pos = line.left(match_index - 1).lastIndexOf(':');
			if (last_colon_pos != -1) {
				if (last_colon_pos != match_index) {
					premessage = line.left(last_colon_pos + 1);
					premessage += ' ';
				}
			}
			std::move(*this) << premessage << Console_Link{sol_message.script_path + ':' + line_number} << sol_message.script_name + ':' + line_number << ": "
							 << post_message;
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
