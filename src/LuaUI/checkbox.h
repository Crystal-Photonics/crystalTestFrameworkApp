#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

#include "ui_container.h"
/** \ingroup ui
 *  \{
 */

// clang-format off
/*!
  \class   CheckBox
  \author Tobias Rieger (tr@crystal-photonics.com),<br> Arne Krüger
  (ak@crystal-photonics.com)
  \brief Checkbox Ui-Element
  \par
    Interface to a checkbox which the lua-script author can use to display a checkbox to the script-user
  */
// clang-format on

class CheckBox : public UI_widget {
public:
#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  CheckBox(string title);
#endif
  ///\cond HIDDEN_SYMBOLS
  CheckBox(UI_container *parent, const std::string text);
  ~CheckBox();
  ///\endcond
  // clang-format off
  /*!     \fn CheckBox(string title)
          \brief Creates a CheckBox
          \param title The text displayed on the checkbox
          \par examples:
          \code
              local checkbox = Ui.CheckBox.new("My Checkbox")
          \endcode
  */
  // clang-format on

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_checked(bool checked);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_checked(const bool checked);
  /// @endcond
  // clang-format off
  /*! \fn set_checked(bool checked)
      \brief Sets the checked state.
      \param checked the check state of the checkbox (true / false)
      \sa CheckBox::get_checked()
      \par examples:
      \code
          local checkbox = Ui.CheckBox.new("My Checkbox")
          checkbox:set_checked(true)   -- checkbox is checked
          checkbox:set_checked(false)  -- checkbox is unchecked
      \endcode
  */
  // clang-format on

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  bool get_checked();
#endif
  /// @cond HIDDEN_SYMBOLS
  bool get_checked() const;
  /// @endcond
  // clang-format off
  /*! \fn bool get_checked()
      \brief Returns the checked state.
      \return the checked state(true / false).
      \sa CheckBox::set_checked()
      \par examples:
      \code
          local checkbox = Ui.CheckBox.new("My Checkbox")
          checkbox:set_checked(true)   -- checkbox is checked
          local is_checked = checkbox:get_checked()
          print("checkbox is checked: "..tostring(is_checked))
      \endcode
  */
  // clang-format on

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_text(string text);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_text(const std::string text);
  /// @endcond
  // clang-format off
  /*! \fn  set_text(string text)
      \brief sets the text of the checkbox.
      \param text the text to be displayed
      \sa CheckBox::get_text()
      \par examples:
      \code
          local checkbox = Ui.CheckBox.new("My Checkbox")  -- checkbox displays "My Checkbox"
          checkbox:set_text("Hallo Welt")   -- checkbox displays "Hallo Welt"
      \endcode
  */
  // clang-format on

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  string get_text();
#endif
  /// @cond HIDDEN_SYMBOLS
  std::string get_text() const;
  /// @endcond
  // clang-format off
  /*! \fn  string get_text()
      \brief Returns the the display text.
      \return the display text (string)
      \sa CheckBox::set_text()
      \par examples:
      \code
          local checkbox = Ui.CheckBox.new("My Checkbox")
          checkbox:set_text("Hallo Welt")
          local checkbox_text = checkbox:get_text()
          print("checkbox displays: "..checkbox_text)
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
      \brief sets the visibility of the checkbox.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the checkbox is hidden
          \li If \c true: the checkbox is visible (default)
      \sa CheckBox::set_enabled()
      \par examples:
      \code
          local checkbox = Ui.CheckBox.new("My Checkbox")
          checkbox:set_visible(false)   -- checkbox is hidden
          checkbox:set_visible(true)   -- checkbox is visible
      \endcode
  */
  // clang-format on

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_enabled(bool enable);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_enabled(bool enable);
  /// @endcond
  // clang-format off
  /*! \fn  set_enabled(bool enable);
      \brief sets the enable state of the checkbox.
      \param enable whether enabled or not. (true / false)
          \li If \c false: the checkbox is disabled and gray
          \li If \c true: the checkbox enabled (default)
      \sa CheckBox::set_visible()
      \par examples:
      \code
          local checkbox = Ui.CheckBox.new("My Checkbox")
          checkbox:set_enabled(false)   -- checkbox is disabled
          checkbox:set_enabled(true)   -- checkbox is enabled
      \endcode
  */
  // clang-format on

private:
  QCheckBox *checkbox = nullptr;
};
/** \} */ // end of group ui
#endif    // CHECKBOX_H
