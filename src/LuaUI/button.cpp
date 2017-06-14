#include "button.h"

#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
Button::Button(UI_container *parent, ScriptEngine *script_engine, const std::string &title)
    : UI_widget{parent}
    , button(new QPushButton(QString::fromStdString(title), parent))
    , script_engine{script_engine} {
    parent->add(button, this);
    button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    pressed_connection = QObject::connect(button, &QPushButton::pressed, [this] { pressed = true; });
    parent->scroll_to_bottom();
}

Button::~Button() {
    QObject::disconnect(pressed_connection);
    button->setEnabled(false);
}
///\endcond
bool Button::has_been_clicked() const {
    return pressed;
}

void Button::await_click() {
    QMetaObject::Connection callback_connection = QObject::connect(button, &QPushButton::pressed, [this] { script_engine->ui_event_queue_send(); });
    script_engine->ui_event_queue_run();
    QObject::disconnect(callback_connection);
}

void Button::set_visible(bool visible) {
    button->setVisible(visible);
}
