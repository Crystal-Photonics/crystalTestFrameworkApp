#ifndef USERINSTRUCTIONLABEL_H
#define USERINSTRUCTIONLABEL_H
#include "scriptengine.h"
#include "ui_container.h"
#include <QMetaObject>
#include <QString>
#include <functional>
#include <string>

class QLabel;
class QSplitter;
class QTimer;
class QCheckBox;
class QHBoxLayout;
class QWidget;
class QLineEdit;
class QPushButton;
class QPushButton;
/** \ingroup ui
 *  \{
\class  UserInstructionLabel
\brief  A UserInstructionLabel is used to instruct or ask the user something. It contains a text and buttons depending whether
    it is an instruction or a question. If it is an instruction a button with the caption "next" is displayed and in case
    of a question two buttons with the caption "yes" and "no" are displayed.

 */
class UserInstructionLabel : public UI_widget {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    UserInstructionLabel(string instruction_text);
#endif
    /// @cond HIDDEN_SYMBOLS
    UserInstructionLabel(UI_container *parent, ScriptEngine *script_engine, std::string instruction_text);
    ~UserInstructionLabel();
    /// @endcond
    // clang-format off
  /*! \fn  UserInstructionLabel(string instruction_text);
      \brief Creates an UserInstructionLabel label.
      \param instruction_text the instruction text.
      \par examples:
      \code
            local uil = Ui.UserInstructionLabel.new("Connect the device with the power supply")
            uil:await_event()
            print("power supply connected")
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_instruction_text(string instruction_text);
#endif
    /// @cond HIDDEN_SYMBOLS
    void set_instruction_text(const std::string &instruction_text);
    /// @endcond
    // clang-format off
  /*! \fn set_instruction_text(string instruction_text);
      \brief Overwrites the instruction text the user sees.
      \param instruction_text the instruction text.
      \par examples:
      \code
            local uil = Ui.UserInstructionLabel.new("Connect the device with the power supply")
            uil:set_instruction_text("Connect the battery.")
            uil:await_event()
            print("power supply connected")
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_visible(bool visible);
#endif
    /// @cond HIDDEN_SYMBOLS
    void set_visible(bool visible);
    /// @endcond
    // clang-format off
  /*! \fn  set_visible(bool visible);
      \brief sets the visibility of the UserInstructionLabel object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the UserInstructionLabel object is hidden
          \li If \c true: the UserInstructionLabel object is visible (default)
      \par examples:
      \code
          local uil = Ui.UserInstructionLabel.new("Hello World")
          uil:set_visible(false)   -- UserInstructionLabel object is hidden
          uil:set_visible(true)   -- UserInstructionLabel object is visible
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_enabled(bool enabled);
#endif
    /// @cond HIDDEN_SYMBOLS
    void set_enabled(bool enabled);
    /// @endcond
    // clang-format off
  /*! \fn  set_enabled(bool enabled);
      \brief sets the enable state of the UserInstructionLabel object.
      \param enabled whether enabled or not. (true / false)
          \li If \c false: the UserInstructionLabel object is disabled and gray
          \li If \c true: the UserInstructionLabel object enabled (default)
      \par examples:
      \code
            local uil = Ui.UserInstructionLabel.new("Hello World")
            uil:set_enabled(false)   --  UserInstructionLabel object is disabled
            uil:set_enabled(true)   --  UserInstructionLabel object is enabled
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    await_event();
#endif
    /// @cond HIDDEN_SYMBOLS
    void await_event();
    /// @endcond
    // clang-format off
  /*! \fn await_event();
      \brief Shows the button next together with the instruction_text. It waits until the user has clicked the button.
      \par examples:
      \code
            local uil = Ui.UserInstructionLabel.new("Connect the device with the power supply")
            uil:await_event()
            print("power supply connected")
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool await_yes_no();
#endif
    /// @cond HIDDEN_SYMBOLS
    bool await_yes_no();
    /// @endcond
    // clang-format off
  /*! \fn bool await_yes_no();
      \brief Shows the buttons yes and no together with the instruction_text. It waits until the user has clicked one button and returns true, if the user clicked "yes"
      \returns true, if the user clicked "yes"
      \par examples:
      \code
            local uil = Ui.UserInstructionLabel.new("Does the red LED blinks?")
            local red_led_blinks = uil:await_yes_no()
            print(red_led_blinks)
      \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void scale_columns();
    ///\endcond

    private:
    ///\cond HIDDEN_SYMBOLS
    QLabel *label_user_instruction = nullptr;
    QTimer *timer = nullptr;

    QPushButton *button_next = nullptr;
    QLabel *label_next = nullptr;

    QPushButton *button_yes = nullptr;
    QLabel *label_yes = nullptr;

    QPushButton *button_no = nullptr;
    QLabel *label_no = nullptr;

    QHBoxLayout *hlayout = nullptr;
    QString instruction_text;
    uint blink_state = 0;
    bool run_hotkey_loop();

    void start_timer();

    bool is_question_mode = false;

    QMetaObject::Connection callback_timer = {};
    QMetaObject::Connection callback_button_yes = {};
    QMetaObject::Connection callback_button_no = {};
    QMetaObject::Connection callback_button_next = {};
    ScriptEngine *script_engine;
    int total_width = 10;
    void resizeMe(QResizeEvent *event) override;
    bool is_init = false;
    ///\endcond
};
/** \} */ // end of group ui
#endif    // USERINSTRUCTIONLABEL_H
