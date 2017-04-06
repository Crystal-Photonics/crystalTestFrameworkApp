#include "button.h"
#include "ui_container.h"

#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
Button::Button(UI_container *parent, const std::string &title)
	: button(new QPushButton(QString::fromStdString(title), parent)) {
	parent->add(button);
	button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    pressed_connection = QObject::connect(button, &QPushButton::pressed, [this] { pressed = true; });
}

Button::~Button() {
    QObject::disconnect(pressed_connection);
    QObject::disconnect(callback_connection);
    button->setEnabled(false);
}
///\endcond
bool Button::has_been_clicked() const {
    return pressed;
}

///\cond HIDDEN_SYMBOLS
void Button::set_single_shot_return_pressed_callback(std::function<void()> callback) {
    callback_connection = QObject::connect(button, &QPushButton::clicked, [&callback_connection = this->callback_connection, callback = std::move(callback) ] {
        callback();
        QObject::disconnect(callback_connection);
    });
}
///\endcond
