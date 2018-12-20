#include "lineedit.h"
#include "ui_container.h"

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
    if (entermode == LineEdit_Entermode::TextMode) {
        QMetaObject::Connection callback_connection =
            QObject::connect(text_edit, &QLineEdit::returnPressed, [this] { this->script_engine->ui_event_queue_send(); });
        script_engine->ui_event_queue_run();
        QObject::disconnect(callback_connection);
    } else {
        throw std::runtime_error(QString("LineEdit: await_return is not available in Datemode.").toStdString());
    }
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
