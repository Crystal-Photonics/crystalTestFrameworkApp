#include "lineedit.h"

#include <QLineEdit>
#include <QSplitter>

LineEdit::LineEdit(QSplitter *parent)
	: edit(new QLineEdit(parent)) {
	parent->addWidget(edit);
}

LineEdit::~LineEdit() {
	edit->setReadOnly(true);
}

void LineEdit::set_placeholder_text(const std::string &text) {
	edit->setPlaceholderText(text.c_str());
}

std::string LineEdit::get_text() const {
	return edit->text().toStdString();
}
