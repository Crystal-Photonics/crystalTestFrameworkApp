#ifndef SPINEDIT_H
#define SPINEDIT_H

#include "ui_container.h"
#include <string>
class QLabel;
class QSplitter;
class QLineEdit;
class QWidget;
class QSpinBox;

/** \ingroup ui
  \{
\class  SpinBox
\brief  A SpinBox is a LineEdit for numbers. It has little buttons (arrow up/down) at the side which can be used to in- or decrement the values in the spinbox.
 */
class SpinBox : public UI_widget {
    public:
    ///\cond HIDDEN_SYMBOLS
    SpinBox(UI_container *parent);
    ~SpinBox();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    SpinBox();
#endif
    // clang-format off
  /*! \fn  SpinBox();
    \brief Creates a spinbox object
    \par examples:
    \code
        local sb = Ui.SpinBox.new()
        local intvalue = sb:get_value()
        print(intvalue) -- prints value
    \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    int get_value();
#endif
    /// \cond HIDDEN_SYMBOLS
    int get_value() const;
    /// \endcond
    // clang-format off
  /*! \fn  int get_value();
    \brief Returns the integer value the user entered.
    \return the value of the spin box.
    \par examples:
    \code
        local sb = Ui.SpinBox.new()
        local intvalue = sb:get_value()
        print(intvalue) -- prints value
    \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_max_value(int value);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_max_value(const int value);
    /// \endcond
    // clang-format off
  /*! \fn  set_max_value(int value);
        \brief Sets max value of an spin box object the user can select
        \param value int value. The max value of the spin box object. default is 100.
        \sa set_value()
        \sa set_min_value()
        \par examples:
        \code
            local sb = Ui.SpinBox.new()
            sb:set_max_value(20) -- The max value
        \endcode

  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_min_value(int value);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_min_value(const int value);
    /// \endcond
    // clang-format off
  /*! \fn  set_min_value(int value);
    \brief Sets the min value of an spin box object the user can select.
    \param value int value. The min value of the spin box object.
    \sa set_value()
    \par examples:
    \code
        local sb = Ui.SpinBox.new()
        sb:set_min_value(1) -- The minimal value
    \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_value(int value);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_value(const int value);
    /// \endcond
    // clang-format off
  /*! \fn  set_value(int value);
    \brief Sets value of an spin box object.
    \param value int value. The value of the spin box object shown to the user.
    \sa set_value()
    \par examples:
    \code
        local sb = Ui.SpinBox.new()
        sb:set_value(20) -- The value the user can edit now
    \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_caption(string caption);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_caption(const std::string &caption);
    /// \endcond
    // clang-format off
  /*! \fn  set_caption(string caption);
    \brief Sets the caption of an spin box object.
    \param caption String value. The caption of the spin box object.
    \details Caption is displayed as a title of the spin box.
    \sa get_caption()
    \par examples:
    \code
        local sb = Ui.SpinBox.new()
        sb:set_caption("TestEdit")
    \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_caption();
#endif
    /// \cond HIDDEN_SYMBOLS
    std::string get_caption() const;
    /// \endcond
    // clang-format off
  /*! \fn  string get_caption();
    \brief Returns the caption.
    \return the caption of the spin box object set by set_caption() as a string value.
    \sa set_caption()
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_visible(bool visible);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_visible(bool visible);
    /// \endcond
    // clang-format off
  /*! \fn  set_visible(bool visible);
      \brief sets the visibility of the spinbox object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the spinbox object is hidden
          \li If \c true: the spinbox object is visible (default)
      \par examples:
      \code
          local spinbox = Ui.SpinBox.new()
          spinbox:set_visible(false)   -- spinbox object is hidden
          spinbox:set_visible(true)   -- spinbox object is visible
      \endcode
  */
    // clang-format on

    private:
    /// \cond HIDDEN_SYMBOLS
    QLabel *label = nullptr;
    QSpinBox *spinbox = nullptr;
    /// \endcond
};
/** \} */ // end of group ui

#endif // SPINEDIT_H
