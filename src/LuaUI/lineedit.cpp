#include "lineedit.h"

#include <QLineEdit>
#include <QSplitter>

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

void LineEdit::set_single_shot_return_pressed_callback(std::function<void()> callback) {
	callback_connection =
		QObject::connect(edit, &QLineEdit::returnPressed, [&callback_connection = this->callback_connection, callback = std::move(callback) ] {
			callback();
			QObject::disconnect(callback_connection);
		});
}
