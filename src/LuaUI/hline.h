#ifndef HLINE_H
#define HLINE_H

#include "ui_container.h"

class QFrame;
class QWidget;
/** \ingroup ui
 *  \{
 */

// clang-format off
/*!
  \class   HLine
  \brief HLine Ui-Element. It paints a horizontal line in the script-ui area. You can use it e.g. to separate topics.
  \image html HLine.png HLine seperator
  \image latex HLine.png HLine seperator
  \image rtf HLine.png HLine seperator
  */
// clang-format on
class HLine : public UI_widget {
    public:
    ///\cond HIDDEN_SYMBOLS
    HLine(UI_container *parent);
    ~HLine();
    ///\endcond

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    HLine();
#endif
    // clang-format off
  /*! \fn  HLine();
      \brief Creates a horizontal line.
      \par examples:
      \code
          local hline = Ui.HLine.new()
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
      \brief sets the visibility of the horizontal line.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the horizontal line is hidden
          \li If \c true: the horizontal line is visible (default)
      \par examples:
      \code
          local hline = Ui.HLine.new()
          hline:set_visible(false)   -- horizontal line is hidden
          hline:set_visible(true)   -- horizontal line is visible
      \endcode
  */

    // clang-format on
    private:
    QFrame *line = nullptr;
};
/** \} */ // end of group ui
#endif    // HLINE_H
