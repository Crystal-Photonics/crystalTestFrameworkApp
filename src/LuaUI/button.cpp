#include "button.h"
#include "util.h"

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

void Button::reset_click_state() {
    pressed = false;
}

void Button::await_click() {
    QMetaObject::Connection callback_connection;
    auto connector = Utility::RAII_do(
        [&callback_connection, this] { callback_connection = QObject::connect(button, &QPushButton::pressed, [this] { script_engine->post_ui_event(); }); },
        [&callback_connection] { QObject::disconnect(callback_connection); });
    script_engine->await_ui_event();
}

void Button::set_visible(bool visible) {
    button->setVisible(visible);
}
