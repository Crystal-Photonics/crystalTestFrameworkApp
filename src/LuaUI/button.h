#ifndef BUTTON_H
#define BUTTON_H

#include "scriptengine.h"
#include "ui_container.h"
#include <QMetaObject>
#include <QPushButton>
#include <functional>
#include <string>

/** \defgroup ui User-intercation
 *  Interface of built-in user interface functions.
 *  \{
 */

// clang-format off
/*!
  \class   Button
  \author Tobias Rieger (tr@crystal-photonics.com),<br> Arne Krüger
  (ak@crystal-photonics.com)
  \brief button Ui-Element
  \par
    Interface to a button which the lua-script author can use to display a button to the script-user.
  */
// clang-format on

struct Button : public UI_widget {
#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  Button(string title);
#endif
  ///\cond HIDDEN_SYMBOLS
  Button(UI_container *parent, ScriptEngine *script_engine,
         const std::string &title);
  ~Button();
  ///\endcond

  /*!     \fn Button(string title)
          \brief Creates a Button
          \param title The text displayed on the button
          \par examples:
          \code
              local button = Ui.Button.new("My Button")
              --wait until button has been clicked:
              button:await_click()
          \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  bool has_been_clicked();
#endif
  /// @cond HIDDEN_SYMBOLS
  bool has_been_clicked() const;
  /// @endcond
  /*!     \fn bool has_been_clicked()
          \brief Returns true if button has been clicked.
          \sa Button::reset_click_state()
          \sa Button::await_click()
          \par examples:
          \code
              local button = Ui.Button.new("My Button")
              --wait until button has been clicked:
              while not button:has_been_clicked() do
                sleep_ms(100);
              end
              if button:has_been_clicked() then
                print("Still remembering: Button has been clicked.")
              end
          \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  reset_click_state();
#endif
  /// @cond HIDDEN_SYMBOLS
  void reset_click_state();
  /// @endcond
  /*!     \fn reset_click_state()
          \brief Lets the button forget whether it has been clicked
          \sa Button::has_been_clicked()
          \par examples:
          \code
              local button = Ui.Button.new("My Button")
              --wait until button has been clicked:
              while not button:has_been_clicked() do
                sleep_ms(100);
              end
              if button:has_been_clicked() then
                print("Still remembering: Button has been clicked.")
              end
              button:reset_click_state()
              if button:has_been_clicked() then --won't enter this block
                print("Still remembering: Button has been clicked.")
              end
          \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  await_click();
#endif
  /// @cond HIDDEN_SYMBOLS
  void await_click();
  /// @endcond

  /*!     \fn await_click()
          \brief blocks script execution until button has been clicked
          \par examples:
          \code
              local button = Ui.Button.new("My Button")
              --wait until button has been pressed:
              print("waiting..")
              button:await_click()
              print("Button has been clicked.")
          \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_visible(bool visible);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_visible(bool visible);
  /// @endcond
  /*! \fn  set_visible(bool visible);
    \brief sets the visibility of the button.
    \param visible the state of the visibility. (true / false)
        \li If \c false: the button is hidden
        \li If \c true: the button is visible (default)
    \sa Button::set_enabled()
    \par examples:
    \code
        local button = Ui.Button.new("My Button")
        button:set_visible(false)   -- button is hidden
        button:set_visible(true)   -- button is visible
    \endcode
*/

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_enabled(bool enable);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_enabled(bool enable);
  /// @endcond
  /*! \fn  set_enabled(bool enable);
    \brief sets the enable state of the button.
    \param enable whether enabled or not. (true / false)
        \li If \c false: the button is disabled and gray
        \li If \c true: the button enabled (default)
    \sa Button::set_visible()
    \par examples:
    \code
        local button = Ui.Button.new("My Button")
        button:set_enabled(false)   -- button is disabled
        button:set_enabled(true)   -- button is enabled
    \endcode
*/

private:
  QPushButton *button = nullptr;
  QMetaObject::Connection pressed_connection;
  bool pressed = false;
  ScriptEngine *script_engine;
};

/** \} */ // end of group ui

#endif // BUTTON_H
