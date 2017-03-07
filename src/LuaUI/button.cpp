#include "button.h"

#include <QPushButton>
#include <QString>
#include <QSplitter>
///\cond HIDDEN_SYMBOLS
Button::Button(QSplitter *parent, const std::string &title)
	: button(new QPushButton(QString::fromStdString(title), parent)) {
	parent->addWidget(button);
	pressed_connection = QObject::connect(button, &QPushButton::pressed, [this] {
		pressed = true;
	});
}

Button::~Button() {
	QObject::disconnect(pressed_connection);
	button->setEnabled(false);
}
///\endcond
bool Button::has_been_pressed() const {
	return pressed;
}
