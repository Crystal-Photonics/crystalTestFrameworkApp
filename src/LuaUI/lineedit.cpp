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
LineEdit::LineEdit(UI_container *parent)
	: label{new QLabel(parent)}
	, edit{new QLineEdit(parent)} {
	QVBoxLayout *layout = new QVBoxLayout;
    label->setVisible(false);
    layout->addWidget(label);
	layout->addWidget(edit, 1, Qt::AlignBottom);
	parent->add(layout);
	label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	edit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

LineEdit::~LineEdit() {
    edit->setReadOnly(true);
    QObject::disconnect(callback_connection);
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
    this->name = name;
}

std::string LineEdit::get_name() const {
    return name;
}

void LineEdit::set_caption(const std::string &caption) {
    label->setText(QString::fromStdString(caption));
    label->setVisible(label->text().size());
}

std::string LineEdit::get_caption() const {
    label->text().toStdString();
}

double LineEdit::get_number() const {
    bool ok = true;
    double retval = edit->text().toDouble(&ok);
    if (ok == false) {
        retval = QInputDialog::getDouble(edit, "Invalid value", "Der Wert \"" + edit->text() + "\" im Feld \"" + QString::fromStdString(name) +
                                                                    "\" ist keine Nummer. Bitte tragen Sie die nach.");
    }
    return retval;
}
///\cond HIDDEN_SYMBOLS
void LineEdit::set_single_shot_return_pressed_callback(std::function<void()> callback) {
    callback_connection =
        QObject::connect(edit, &QLineEdit::returnPressed, [&callback_connection = this->callback_connection, callback = std::move(callback) ] {
            callback();
            QObject::disconnect(callback_connection);
        });
}
///\endcond
