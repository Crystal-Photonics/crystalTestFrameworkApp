///\cond HIDDEN_SYMBOLS

#include "userwaitlabel.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "ui_container.h"
#include "util.h"

#include <QCheckBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMovie>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

constexpr auto updater_user_data = "updater_user_data";

static QString ms_to_display_string(int time_ms) {
	return QString{"%1.%2"}.arg(time_ms / 1000).arg(time_ms / 100 % 10);
}

UserWaitLabel::UserWaitLabel(UI_container *parent, ScriptEngine *script_engine, std::string extra_explanation)
    : UI_widget{parent}
    , label_user_instruction{new QLabel(parent)}
    , timer{new QTimer(parent)}
    , instruction_text{QString::fromStdString(extra_explanation)}
    , script_engine(script_engine)

{
	assert(currently_in_gui_thread());
    hlayout = new QHBoxLayout;

	hlayout->addWidget(label_user_instruction);

    spinner_label = new QLabel(parent);
    QMovie *movie = new QMovie("://src/LuaUI/ajax-loader.gif");
    spinner_label->setMovie(movie); //":/MyPreciousRes/MyResources.pro"
    movie->start();

	hlayout->addWidget(spinner_label);

    parent->add(hlayout, this);

    label_user_instruction->setText(QString::fromStdString(extra_explanation));
    label_user_instruction->setWordWrap(true);

    start_timer();
    timer->start(500);

    set_enabled(true);
    parent->scroll_to_bottom();

	progress_bar = new QProgressBar{parent};
	progress_bar->setVisible(false);
	hlayout->addWidget(progress_bar);

	QObject::connect(script_engine, &ScriptEngine::script_interrupted, [progress_bar = this->progress_bar] { disable_progress_bar_and_updater(progress_bar); });
	QObject::connect(script_engine, &ScriptEngine::script_finished, [progress_bar = this->progress_bar] { disable_progress_bar_and_updater(progress_bar); });
}

UserWaitLabel::~UserWaitLabel() {
	assert(currently_in_gui_thread());
	timer->stop();
    QObject::disconnect(callback_timer);

    set_enabled(false);
}

bool UserWaitLabel::run_hotkey_loop() {
	assert(not currently_in_gui_thread());
	Utility::promised_thread_call(MainWindow::mw, [this] {
		assert(currently_in_gui_thread());
        set_enabled(true);
    });
	return script_engine->await_hotkey_event() == Event_id::Hotkey_confirm_pressed;
}

void UserWaitLabel::set_text(const std::string &instruction_text) {
	assert(currently_in_gui_thread());
    this->instruction_text = QString::fromStdString(instruction_text);
	label_user_instruction->setText(this->instruction_text);
}

void UserWaitLabel::sleep_ms(int duration_ms) {
	assert(not currently_in_gui_thread());
	Utility::thread_call(MainWindow::mw, [this, duration_ms] { show_progress_timer_ms(duration_ms); });

	//actually sleep
	if (script_engine->await_timeout(std::chrono::milliseconds{duration_ms}, std::chrono::milliseconds{0}) != Event_id::Timer_expired) {
		//Got interrupted or something, didn't run to the end
		Utility::thread_call(MainWindow::mw, [progress_bar = this->progress_bar] { disable_progress_bar_and_updater(progress_bar); });
	}
}

void UserWaitLabel::show_progress_timer_ms(int duration_ms) {
	assert(currently_in_gui_thread());
	show_progress_timer_ms_impl(duration_ms, 0);
}

void UserWaitLabel::set_current_progress_ms(int position_ms) {
	assert(currently_in_gui_thread());
	show_progress_timer_ms_impl(last_progressbar_duration_ms, position_ms);
}

void UserWaitLabel::set_enabled(bool enabled) {
	assert(currently_in_gui_thread());

    label_user_instruction->setEnabled(enabled);
    label_user_instruction->setText(instruction_text);

    spinner_label->setVisible(enabled);

    if (enabled) {
        timer->start(500);
    } else {
        timer->stop();
		disable_progress_bar_and_updater(progress_bar);
    }
}

void UserWaitLabel::start_timer() {
    callback_timer = QObject::connect(timer, &QTimer::timeout, [this]() {
		assert(currently_in_gui_thread());
        if (blink_state == Globals::ui_blink_ratio) {
			const auto old_width = label_user_instruction->width();
			label_user_instruction->setMinimumWidth(old_width);
			label_user_instruction->setText("");
            blink_state = 0;
        } else {
			label_user_instruction->setMinimumWidth(0);
            label_user_instruction->setText(instruction_text);
        }
        blink_state++;
	});
}

void UserWaitLabel::disable_progress_bar_and_updater(QProgressBar *progress_bar) {
	Utility::thread_call(MainWindow::mw, [progress_bar] {
		progress_bar->setProperty(updater_user_data, QVariant::fromValue(progress_bar->property(updater_user_data).toUInt() + 1));
		progress_bar->setVisible(false);
	});
}

void UserWaitLabel::show_progress_timer_ms_impl(int duration_ms, int start) {
	assert(currently_in_gui_thread());
	last_progressbar_duration_ms = duration_ms;
	const auto max_value_ms = duration_ms;
	const auto end = std::chrono::system_clock::now() + std::chrono::milliseconds{duration_ms - start};
	progress_bar->setMaximum(max_value_ms);
	progress_bar->setVisible(true);
	const auto progress_value = progress_bar->property(updater_user_data).toUInt() + 1;
	progress_bar->setProperty(updater_user_data, QVariant::fromValue(progress_value));
	//hack to enable recursive lambda
	auto updater = [end, max_value_ms, progress_bar = this->progress_bar, progress_value](auto self) -> void {
		if (progress_bar->property(updater_user_data).toUInt() != progress_value) {
			return;
		}
		const auto time_passed_ms = max_value_ms - std::chrono::duration_cast<std::chrono::milliseconds>(end - std::chrono::system_clock::now()).count();
		if (time_passed_ms < max_value_ms) {
			progress_bar->setValue(time_passed_ms);
			progress_bar->setFormat(QString{"%1/%2s"}.arg(ms_to_display_string(time_passed_ms)).arg(ms_to_display_string(max_value_ms)));
			const auto sleep_time_ms = std::min(16, static_cast<int>(max_value_ms - time_passed_ms) - 1);
			QTimer::singleShot(sleep_time_ms, [self] { self(self); });
		} else {
			progress_bar->setVisible(false);
		}
	};
	updater(updater);
}
///\endcond
