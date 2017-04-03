#include "button.h"

#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>

///\cond HIDDEN_SYMBOLS
Button::Button(QSplitter *parent, const std::string &title) {
    base_widget = new QWidget(parent);
    button = new QPushButton(QString::fromStdString(title), base_widget);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(button);
    base_widget->setLayout(layout);
    parent->addWidget(base_widget);

    base_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

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
