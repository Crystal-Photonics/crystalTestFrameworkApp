#ifndef BUTTON_H
#define BUTTON_H

#include <functional>

#include <QMetaObject>

class QPushButton;
class QSplitter;

/** \defgroup ui User-intercation
 *  Interface of built-in user interface functions.
 *  \{
 */

struct Button {
    ///\cond HIDDEN_SYMBOLS
    Button(QSplitter *parent, const std::string &title);
    ///\endcond
    ~Button();

    bool has_been_pressed() const; //!<\brief Returns true if button has been pressed.
                                   //!< \par examples:
                                   //!< \code
                                   //!  local button = Ui.Button.new("My Button")
                                   //! --wait until button has been pressed:
                                   //!  while not button:has_been_pressed() do
                                   //!      sleep_ms(100);
                                   //!  end
                                   //! \endcode

    private:
    QPushButton *button = nullptr;
    QMetaObject::Connection pressed_connection;
    bool pressed = false;
};

/** \} */ // end of group ui

#endif // BUTTON_H
