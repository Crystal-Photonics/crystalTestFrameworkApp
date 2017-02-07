#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include <QMetaObject>

class QPushButton;
class QSplitter;

struct Button {
	Button(QSplitter *parent, const std::string &title);
	~Button();

	bool has_been_pressed() const;

	private:
	QPushButton *button = nullptr;
	QMetaObject::Connection pressed_connection;
	bool pressed = false;
};

#endif // BUTTON_H
