#include "button.h"

#include <QPushButton>
#include <QString>
#include <QSplitter>

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

bool Button::has_been_pressed() const {
	return pressed;
}
