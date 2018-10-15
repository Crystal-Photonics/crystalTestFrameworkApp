#include "lineedit.h"
#include "ui_container.h"

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
    , edit{new QLineEdit(parent)}
    , script_engine{script_engine} {
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(edit, 0, Qt::AlignBottom);
    layout->addStretch(1);
    parent->add(layout, this);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    edit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    parent->scroll_to_bottom();
}

LineEdit::~LineEdit() {
    edit->setReadOnly(true);
}
///\endcond
void LineEdit::set_placeholder_text(const std::string &text) {
    edit->setPlaceholderText(text.c_str());
}

std::string LineEdit::get_text() const {
    return edit->text().toStdString();
}

void LineEdit::set_text(const std::string &text) {
    edit->setText(QString::fromStdString(text));
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
    edit->setVisible(visible);
}

void LineEdit::set_enabled(bool enabled) {
    label->setEnabled(enabled);
    edit->setEnabled(enabled);
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
    edit->setFocus();
}

void LineEdit::await_return() {
    QMetaObject::Connection callback_connection = QObject::connect(edit, &QLineEdit::returnPressed, [this] { this->script_engine->ui_event_queue_send(); });
    script_engine->ui_event_queue_run();
    QObject::disconnect(callback_connection);
}

double LineEdit::get_number() const {
    bool ok = true;
    double retval = edit->text().toDouble(&ok);
    if (ok == false) {
        retval = QInputDialog::getDouble(edit, "Invalid value", "Der Wert \"" + edit->text() + "\" im Feld \"" + QString::fromStdString(name_m) +
                                                                    "\" ist keine Nummer. Bitte tragen Sie die nach.");
    }
    return retval;
}
///\cond HIDDEN_SYMBOLS

///\endcond
