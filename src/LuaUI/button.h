#ifndef BUTTON_H
#define BUTTON_H

#include <QMetaObject>
#include <functional>
#include <string>
#include <QPushButton>


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

    bool has_been_clicked() const; //!<\brief Returns true if button has been pressed.
                                   //!< \par examples:
                                   //!< \code
                                   //!  local button = Ui.Button.new("My Button")
                                   //! --wait until button has been pressed:
                                   //!  while not button:has_been_pressed() do
                                   //!      sleep_ms(100);
                                   //!  end
                                   //! \endcode

    void set_single_shot_return_pressed_callback(std::function<void()> callback);

    private:
    QPushButton *button = nullptr;
    QWidget *base_widget = nullptr;
    QMetaObject::Connection pressed_connection;
    bool pressed = false;
    QMetaObject::Connection callback_connection = {};
};

/** \} */ // end of group ui

#endif // BUTTON_H
