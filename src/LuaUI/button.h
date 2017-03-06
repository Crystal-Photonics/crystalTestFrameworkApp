#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include <QMetaObject>

class QPushButton;
class QSplitter;

struct Button {
    ///\cond HIDDEN_SYMBOLS
    Button(QSplitter *parent, const std::string &title);
    ///\endcond
    ~Button();

    bool has_been_pressed() const; //!<\brief Returns true if button has been pressed.
                                   //!< \par examples:
                                   //!< \code
                                   //!  local button = Ui.Button.new("My Button")
                                   //!  while not button:has_been_pressed() do --wait until button has been pressed.
                                   //!      sleep_ms(100);
                                   //!  end
                                   //! \endcode

    private:
    QPushButton *button = nullptr;
    QMetaObject::Connection pressed_connection;
    bool pressed = false;
};

#endif // BUTTON_H
