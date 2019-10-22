///\cond HIDDEN_SYMBOLS
#include "lineedit.h"
#include "Windows/mainwindow.h"
#include "ui_container.h"
#include "util.h"
#include <QDateTime>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

LineEdit::LineEdit(UI_container *parent, ScriptEngine *script_engine)
    : UI_widget{parent}
    , label{new QLabel(parent)}
    , text_edit{new QLineEdit(parent)}
    , date_edit{new QDateEdit(parent)}
    , script_engine{script_engine}
    , pattern_check_m(PatternCheck::None) {
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(text_edit, 0, Qt::AlignBottom);
    layout->addWidget(date_edit, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    text_edit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    date_edit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    set_text_mode();
    parent->scroll_to_bottom();

    callback_text_changed = QObject::connect(text_edit, &QLineEdit::textEdited, [this](const QString &text) {
        auto palette = text_edit->palette();
        if (!pattern_check_m.is_input_matching_to_pattern(text)) {
            palette.setColor(QPalette::Text, Qt::red);
            text_edit->setToolTip(QObject::tr("Pattern must match: ") + pattern_check_m.to_string() + "\n" +
                                  pattern_check_m.example_matching_to_pattern().join("\n"));
        } else {
            text_edit->setToolTip("");
            palette.setColor(QPalette::Text, Qt::black);
        }

        text_edit->setPalette(palette);
        assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    });
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

LineEdit::~LineEdit() {
    text_edit->setReadOnly(true);
    date_edit->setReadOnly(true);
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    QObject::disconnect(callback_text_changed);
}

void LineEdit::set_date_mode() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    entermode = LineEdit_Entermode::DateMode;
    text_edit->setVisible(false);
    date_edit->setVisible(true);
}

void LineEdit::set_text_mode() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    entermode = LineEdit_Entermode::TextMode;
    text_edit->setVisible(true);
    date_edit->setVisible(false);
}

void LineEdit::set_placeholder_text(const std::string &text) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    if (entermode == LineEdit_Entermode::DateMode) {
        throw std::runtime_error(QString("LineEdit:set_placeholder_text is not available in DateMode.").toStdString());
    }
    text_edit->setPlaceholderText(text.c_str());
}

void LineEdit::set_input_check(const std::string &text) {
    pattern_check_m = PatternCheck(QString::fromStdString(text));
}

std::string LineEdit::get_text() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    if (entermode == LineEdit_Entermode::TextMode) {
        while (!pattern_check_m.is_input_matching_to_pattern(text_edit->text())) {
            QString retval = QInputDialog::getText(text_edit, "Invalid value",
                                                   QObject::tr("The value \"%1\" entered in \"%2\" is not valid. Please enter a correct one.\nExplanation: %3")
                                                       .arg(text_edit->text())
                                                       .arg(QString::fromStdString(name_m))
                                                       .arg(pattern_check_m.example_matching_to_pattern().join("\n")));
            text_edit->setText(retval);
        }

        return text_edit->text().toStdString();
    } else if (entermode == LineEdit_Entermode::DateMode) {
        return date_edit->date().toString(date_formatstring).toStdString();
    } else {
        throw std::runtime_error(QString("LineEdit:get_text unknown mode.").toStdString());
    }
}

double LineEdit::get_date() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    if (entermode == LineEdit_Entermode::TextMode) {
        throw std::runtime_error(QString("LineEdit:get_date is not available in Textmode.").toStdString());
    }
    QDateTime date_(date_edit->date());
    return date_.toSecsSinceEpoch();
}

void LineEdit::set_text(const std::string &text) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    set_text_mode();
    text_edit->setText(QString::fromStdString(text));
}

void LineEdit::set_date(double date_value_since_epoch) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    set_date_mode();
    QDateTime date_;
    date_.setSecsSinceEpoch((uint64_t)date_value_since_epoch);
    date_edit->setDate(date_.date());
}

void LineEdit::set_name(const std::string &name) {
    this->name_m = name;
}

std::string LineEdit::get_name() const {
    return name_m;
}

void LineEdit::set_caption(const std::string &caption) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    label->setText(QString::fromStdString(caption));
    //label->setVisible(label->text().size());
    if (caption == "") {
        label->setText(" ");
    }
}

void LineEdit::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    label->setVisible(visible);
    text_edit->setVisible(visible);
    date_edit->setVisible(visible);
}

void LineEdit::set_enabled(bool enabled) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    label->setEnabled(enabled);
    text_edit->setEnabled(enabled);
    date_edit->setEnabled(enabled);
}

bool PatternCheck::is_input_matching_to_pattern(const QString &string_under_test) const {
    auto check_yy_ww_ = [](QString text_under_test) {
        auto sl = text_under_test.split("/");
        if (sl.count() != 2) {
            return false;
        }
        bool ok = true;
        int year = sl[0].toInt(&ok);
        if (ok == false) {
            return false;
        }
        QString week_str = sl[1];
        int week_str_count = week_str.count();
        if ((week_str_count != 2) && (week_str_count != 3)) {
            return false;
        }
        if (week_str.count() == 3) {
            if (!week_str.right(1)[0].isLetter()) {
                return false;
            }
            week_str = week_str.left(2);
        }

        int week = week_str.toInt(&ok);
        if (ok == false) {
            return false;
        }
        if (year <= 17) {
            return false;
        }

        int current_year = QDateTime::currentDateTime().date().year() - 2000;
        int current_week = QDateTime::currentDateTime().date().weekNumber();

        if (year > current_year) {
            return false;
        }
        if ((week > current_week) && (year == current_year)) {
            return false;
        }
        return true;
    };

    switch (t) {
        case None: {
            return true;
        }
        case yyww_m: {
            QString text_under_test = string_under_test;
            if (text_under_test == "-") {
                return true;
            }
            return check_yy_ww_(text_under_test);
        }
        case yyww: {
            return check_yy_ww_(string_under_test);
        }
    }

    return false;
}

std::string LineEdit::get_caption() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return label->text().toStdString();
}

void LineEdit::load_from_cache() {
#if 0
    if (name_m == "") {
        throw std::runtime_error("Lineedit " + caption_m + " has no name but needs one in order to load content from cache.");
    }
    if (parent->user_entry_cache.key_already_used(UserEntryCache::AccessDirection::read_access, QString::fromStdString(name_m))) {
        throw std::runtime_error("Lineedit " + caption_m + " uses the already used field name: \"" + name_m + "\" in order to load content from cache.");
    }
    QString value = parent->user_entry_cache.get_value(QString::fromStdString(name_m));
    edit->setText(value);
#endif
}

void LineEdit::save_to_cache() {
#if 0
    if (name_m == "") {
        throw std::runtime_error("Lineedit " + caption_m + " has no name but needs one in order to store content to cache.");
    }
    if (parent->user_entry_cache.key_already_used(UserEntryCache::AccessDirection::read_access, QString::fromStdString(name_m))) {
        throw std::runtime_error("Lineedit " + caption_m + " uses the already used field name: \"" + name_m + "\" in order to load content from cache.");
    }
    parent->user_entry_cache.set_value(QString::fromStdString(name_m), edit->text());
#endif
}

void LineEdit::set_focus() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    if (entermode == LineEdit_Entermode::TextMode) {
        text_edit->setFocus();
    } else if (entermode == LineEdit_Entermode::DateMode) {
        date_edit->setFocus();
    }
}

void LineEdit::await_return() {
    if (entermode == LineEdit_Entermode::DateMode) {
        throw std::runtime_error("LineEdit::await_return is only available in TextMode, not in DateMode");
    }

    QMetaObject::Connection callback_connection;
    auto connector =
        Utility::RAII_do([&callback_connection,
                          this] { callback_connection = QObject::connect(text_edit, &QLineEdit::returnPressed, [this] { script_engine->post_ui_event(); }); },
                         [&callback_connection] { QObject::disconnect(callback_connection); });
    script_engine->await_ui_event();
}

double LineEdit::get_number() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    if (entermode == LineEdit_Entermode::TextMode) {
        bool ok = true;
        double retval = text_edit->text().toDouble(&ok);
        if (ok == false) {
            retval = QInputDialog::getDouble(
                text_edit, "Invalid value",
                QObject::tr("The value \"%1\" of \"%2\" is not a number. Please enter a number.").arg(text_edit->text()).arg(QString::fromStdString(name_m)));
        }
        return retval;
    } else {
        throw std::runtime_error(QString("LineEdit:get_number is not available in Datemode.").toStdString());
    }
    return INT_MAX;
}

///\endcond
