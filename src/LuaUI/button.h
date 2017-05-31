#ifndef BUTTON_H
#define BUTTON_H

#include <QMetaObject>
#include <functional>
#include <string>
#include <QPushButton>
#include "scriptengine.h"
#include "ui_container.h"

/** \defgroup ui User-intercation
 *  Interface of built-in user interface functions.
 *  \{
 */

struct Button : public UI_widget{
    ///\cond HIDDEN_SYMBOLS
    Button(UI_container *parent, ScriptEngine* script_engine,  const std::string &title);
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

    void await_click();


public:
    void set_visible(bool visible);
private:
    QPushButton *button = nullptr;
    QMetaObject::Connection pressed_connection;
    bool pressed = false;
    ScriptEngine* script_engine;

};

/** \} */ // end of group ui

#endif // BUTTON_H
