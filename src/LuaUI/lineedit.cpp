#include "lineedit.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QSplitter>
#include <QString>

LineEdit::LineEdit(QSplitter *parent)
	: edit(new QLineEdit(parent)) {
	parent->addWidget(edit);
}

LineEdit::~LineEdit() {
	edit->setReadOnly(true);
	QObject::disconnect(callback_connection);
}

void LineEdit::set_placeholder_text(const std::string &text) {
	edit->setPlaceholderText(text.c_str());
}

std::string LineEdit::get_text() const {
    return edit->text().toStdString();
}

void LineEdit::set_text(const std::string &text) {
    edit->setText(QString::fromStdString(text));
}

void LineEdit::set_name(const std::string &text) {
    name = text;
}

std::string LineEdit::get_name() const {
    return name;
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

void LineEdit::set_single_shot_return_pressed_callback(std::function<void()> callback) {
	callback_connection =
		QObject::connect(edit, &QLineEdit::returnPressed, [&callback_connection = this->callback_connection, callback = std::move(callback) ] {
			callback();
			QObject::disconnect(callback_connection);
		});
}
