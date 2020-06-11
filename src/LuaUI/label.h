#ifndef LABEL_H
#define LABEL_H

#include "ui_container.h"
#include <string>

class QLabel;
class QWidget;

/** \ingroup ui
 *  \{
 */

// clang-format off
/*!
  \class   Label
  \brief Label Ui-Element. It displays text in the script-ui area. The text is not editable by the script-user directly.
  \image html Label.png Text label example
  \image latex Label.png Text label example
  \image rtf Label.png Text label example
  */
// clang-format on
class Label : public UI_widget {
    public:
    ///\cond HIDDEN_SYMBOLS
    Label(UI_container *parent, const std::string text);
    ~Label();
    ///\endcond

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    Label(string text);
#endif
    // clang-format off
/*! \fn  Label(string text);
    \brief Creates a Text object.
    \param text The text which will be displayed.
    \par examples:
    \code
        local label = Ui.Label.new("Hello World")
    \endcode
*/
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_text(const std::string text);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_text(string text);
#endif
    // clang-format off
/*! \fn  set_text(string text);
    \brief Overwrites the display text of the label object.
    \param text The text which will be displayed.
    \par examples:
    \code
        local label = Ui.Label.new("Hello World")
        label:set_text("New text")
    \endcode
*/
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    std::string get_text() const;
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_text();
#endif
    // clang-format off
/*! \fn  string get_text();
    \brief Returns the text of the text object.
    \returns the text of the text object.
    \par examples:
    \code
        local label = Ui.Label.new("Hello World")
        local text_variable = label:get_text()
        print(text_variable) --prints: "Hello World"
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
      \brief sets the visibility of the label object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the label object is hidden
          \li If \c true: the label object is visible (default)
      \par examples:
      \code
          local label = Ui.Label.new("Hello World")
          label:set_visible(false)   -- label object is hidden
          label:set_visible(true)   -- label object is visible
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
      \brief sets the enable state of the label object.
      \param enable whether enabled or not. (true / false)
          \li If \c false: the label object is disabled and gray
          \li If \c true: the label object enabled (default)
      \par examples:
      \code
            local label = Ui.Label.new("Hello World")
            label:set_enabled(false)   --  label object is disabled
            label:set_enabled(true)   --  label object is enabled
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_font_size(bool big_font);
#endif
    /// @cond HIDDEN_SYMBOLS
    void set_font_size(bool big_font);
    /// @endcond
    // clang-format off
  /*! \fn set_font_size(bool big_font);
      \brief defines whether the text has a big or a normal font size.
      \param big_font whether big or normal. (true / false)
          \li If \c false: the font size is normal(default)
          \li If \c true: the font size is big.
      \par examples:
      \code
            local label = Ui.Label.new("Hello World")
            label:set_font_size(true)   --  label shows Hello World in big letters
      \endcode
  */
    // clang-format on
    private:
    ///\cond HIDDEN_SYMBOLS
    QLabel *label = nullptr;
    int normal_font_size;
    ///\endcond
};
/** \} */ // end of group ui
#endif    // LABEL_H
