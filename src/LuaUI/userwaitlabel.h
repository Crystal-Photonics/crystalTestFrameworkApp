#ifndef USERWAITLABEL_H
#define USERWAITLABEL_H

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
class QProgressBar;

/** \ingroup ui
   \{

\class  UserWaitLabel
\brief  An UserWaitLabel is an ordinary text label together with an moving
 <a href="https://www.google.com/search?q=ajax+spinner&tbm=isch"> ajax spinner</a> you can use it
 to indicate a waiting state.
 */
class UserWaitLabel : public UI_widget {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    UserWaitLabel(string instruction_text);
#endif
    /// @cond HIDDEN_SYMBOLS
    UserWaitLabel(UI_container *parent, ScriptEngine *script_engine, std::string instruction_text);
    ~UserWaitLabel();
    /// @endcond
    // clang-format off
  /*! \fn  UserWaitLabel(string instruction_text);
      \brief Creates an UserWaitLabel object with instruction_text as label-text.
      \param instruction_text the text
      \par examples:
      \code
          local uwl = Ui.UserWaitLabel.new("Wait until device is connected..")
		  uwl:sleep_ms(1000)
          uwl:set_text("Wait until device is charged..")
		  uwl:sleep_ms(1000)
          uwl:set_enabled(false)
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
	// this block is just for ducumentation purpose
	set_text(string instruction_text);
#endif
	/// @cond HIDDEN_SYMBOLS
	void set_text(const std::string &instruction_text);
	/// @endcond
	// clang-format off
  /*! \fn  set_text(string instruction_text);
	  \brief Overwrites the text of the UserWaitLabel object.
	  \param instruction_text the text
	  \par examples:
	  \code
		  local uwl = Ui.UserWaitLabel.new("Wait until device is connected..")
		  uwl:sleep_ms(1000)
		  uwl:set_text("Wait until device is charged..")
		  uwl:sleep_ms(1000)
		  uwl:set_enabled(false)
	  \endcode
  */
	// clang-format on

#ifdef DOXYGEN_ONLY
	// this block is just for ducumentation purpose
	sleep_ms(number duration_ms);
#endif
	/// @cond HIDDEN_SYMBOLS
	void sleep_ms(int duration_ms);
	/// @endcond
	// clang-format off
  /*! \fn  sleep_ms(number duration_ms);
	  \brief Sleeps for duration_ms milliseconds.
	  \param duration_ms the time to sleep in milliseconds
	  \par example:
	  \code
		  local uwl = Ui.UserWaitLabel.new("Wait until device is charged..")
		  uwl:sleep_ms(1000)
	  \endcode
  */
	// clang-format on

#ifdef DOXYGEN_ONLY
	// this block is just for ducumentation purpose
	show_progress_timer_ms(number duration_ms);
#endif
	/// @cond HIDDEN_SYMBOLS
	void show_progress_timer_ms(int duration_ms);
	/// @endcond
	// clang-format off
  /*! \fn  show_progress_timer_ms(number duration_ms);
	  \brief Displays a progress bar for duration_ms milliseconds.
	  \param duration_ms the time the progress bar needs to get to 100% in milliseconds
	  \par example:
	  \code
		  local uwl = Ui.UserWaitLabel.new("Wait until device is charged..")
		  uwl:show_progress_timer_ms(1000)
		  --do work while progress bar is running
	  \endcode
  */
	// clang-format on

#ifdef DOXYGEN_ONLY
	// this block is just for ducumentation purpose
	set_current_progress_ms(number position_ms);
#endif
	/// @cond HIDDEN_SYMBOLS
	void set_current_progress_ms(int position_ms);
	/// @endcond
	// clang-format off
  /*! \fn  set_current_progress_ms(number position_ms);
	  \brief Moves the progress bar to position_ms milliseconds.
	  \param position_ms the time the progress bar progresses to
	  \par example:
	  \code
		  local uwl = Ui.UserWaitLabel.new("Wait until device is charged..")
		  uwl:show_progress_timer_ms(5000)
		  --do work while progress bar is running
		  --We got done faster or slower than expected, so jump to the right position
		  uwl:set_current_progress_ms(3000)
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
      \brief sets the visibility of the UserWaitLabel object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the UserWaitLabel object is hidden
          \li If \c true: the UserWaitLabel object is visible (default)
      \par examples:
      \code
          local uwl = Ui.UserWaitLabel.new("Hello World")
          uwl:set_visible(false)   -- UserWaitLabel object is hidden
          uwl:set_visible(true)   -- UserWaitLabel object is visible
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
      \brief sets the enable state of the UserWaitLabel object.
      \param enabled whether enabled or not. (true / false)
          \li If \c false: the UserWaitLabel object is disabled and gray
          \li If \c true: the UserWaitLabel object enabled (default)
      \par examples:
      \code
            local uwl = Ui.UserWaitLabel.new("Hello World")
            uwl:set_enabled(false)   --  UserWaitLabel object is disabled
            uwl:set_enabled(true)   --  UserWaitLabel object is enabled
      \endcode
  */
	// clang-format on
	private:
    ///\cond HIDDEN_SYMBOLS
    QLabel *label_user_instruction = nullptr;
    QLabel *spinner_label = nullptr;
    QTimer *timer = nullptr;
	QProgressBar *progress_bar = nullptr;

    QHBoxLayout *hlayout = nullptr;
    QString instruction_text;
    uint blink_state = 0;
    bool run_hotkey_loop();
    void start_timer();
	static void disable_progress_bar_and_updater(QProgressBar *progress_bar);
	void show_progress_timer_ms_impl(int duration_ms, int start);
	int last_progressbar_duration_ms = 0;

    bool is_question_mode = false;

    QMetaObject::Connection callback_timer = {};

    ScriptEngine *script_engine;
    ///\endcond
};
/** \} */ // end of group ui
#endif    // USERINSTRUCTIONLABEL_H
