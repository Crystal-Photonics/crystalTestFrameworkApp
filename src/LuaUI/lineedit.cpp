#include "lineedit.h"
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

///\cond HIDDEN_SYMBOLS
LineEdit::LineEdit(UI_container *parent, ScriptEngine *script_engine)
    : UI_widget{parent}
    , label{new QLabel(parent)}
    , text_edit{new QLineEdit(parent)}
    , date_edit{new QDateEdit(parent)}
    , script_engine{script_engine} {
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
}

LineEdit::~LineEdit() {
    text_edit->setReadOnly(true);
    date_edit->setReadOnly(true);
}

void LineEdit::set_date_mode() {
    entermode = LineEdit_Entermode::DateMode;
    text_edit->setVisible(false);
    date_edit->setVisible(true);
}

void LineEdit::set_text_mode() {
    entermode = LineEdit_Entermode::TextMode;
    text_edit->setVisible(true);
    date_edit->setVisible(false);
}

///\endcond
void LineEdit::set_placeholder_text(const std::string &text) {
    if (entermode == LineEdit_Entermode::DateMode) {
        throw std::runtime_error(QString("LineEdit:set_placeholder_text is not available in DateMode.").toStdString());
    }
    text_edit->setPlaceholderText(text.c_str());
}

void LineEdit::set_input_check(const std::string &text) {
    pattern_check_m = QString::fromStdString(text);
}

std::string LineEdit::get_text() const {
    if (entermode == LineEdit_Entermode::TextMode) {
        return text_edit->text().toStdString();
    } else if (entermode == LineEdit_Entermode::DateMode) {
        return date_edit->date().toString(date_formatstring).toStdString();
    } else {
        throw std::runtime_error(QString("LineEdit:get_text unknown mode.").toStdString());
    }
}

double LineEdit::get_date() const {
    if (entermode == LineEdit_Entermode::TextMode) {
        throw std::runtime_error(QString("LineEdit:get_date is not available in Textmode.").toStdString());
    }
    QDateTime date_(date_edit->date());
    return date_.toSecsSinceEpoch();
}

void LineEdit::set_text(const std::string &text) {
    set_text_mode();
    text_edit->setText(QString::fromStdString(text));
}

void LineEdit::set_date(double date_value_since_epoch) {
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
    label->setText(QString::fromStdString(caption));
    //label->setVisible(label->text().size());
    if (caption == "") {
        label->setText(" ");
    }
}

void LineEdit::set_visible(bool visible) {
    label->setVisible(visible);
    text_edit->setVisible(visible);
    date_edit->setVisible(visible);
}

void LineEdit::set_enabled(bool enabled) {
    label->setEnabled(enabled);
    text_edit->setEnabled(enabled);
    date_edit->setEnabled(enabled);
}

bool LineEdit::is_input_matching_to_pattern() {
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
        if ((week_str.count() != 2) && (week_str.count() != 3)) {
            return false;
        }
        if (week_str.count() == 3) {
            if (!week_str.right(1)[0].isLetterOrNumber()) {
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

        int current_year = QDateTime::currentDateTime().date().year();
        int current_week = QDateTime::currentDateTime().date().weekNumber();

        if (year > current_year) {
            return false;
        }
        if ((week > current_week) && (year == current_year)) {
            return false;
        }
        return true;
    };

    bool minus_allowed = false;
    if (pattern_check_m == "YY/WW?-") {
        QString text_under_test = text_edit->text();
        if (text_under_test == "-") {
            return true;
        }
        return check_yy_ww_(text_under_test);
    }
    if (pattern_check_m == "YY/WW?") {
        minus_allowed = false;
        return check_yy_ww_(text_edit->text());
    }
    return true;
}

std::string LineEdit::get_caption() const {
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
    if (entermode == LineEdit_Entermode::TextMode) {
        text_edit->setFocus();
    } else if (entermode == LineEdit_Entermode::DateMode) {
        date_edit->setFocus();
    }
}

void LineEdit::await_return() {
    if (entermode != LineEdit_Entermode::DateMode) {
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
    if (entermode == LineEdit_Entermode::TextMode) {
        bool ok = true;
        double retval = text_edit->text().toDouble(&ok);
        if (ok == false) {
            retval = QInputDialog::getDouble(text_edit, "Invalid value", "Der Wert \"" + text_edit->text() + "\" im Feld \"" + QString::fromStdString(name_m) +
                                                                             "\" ist keine Nummer. Bitte tragen Sie die nach.");
        }
        return retval;
    } else {
        throw std::runtime_error(QString("LineEdit:get_number is not available in Datemode.").toStdString());
    }
    return INT_MAX;
}
///\cond HIDDEN_SYMBOLS

///\endcond
