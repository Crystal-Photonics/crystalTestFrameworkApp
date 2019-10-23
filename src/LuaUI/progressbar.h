#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "ui_container.h"
#include <string>
class QLabel;
class QSplitter;
class QLineEdit;
class QWidget;
class QProgressBar;

/** \ingroup ui
    \class  ProgressBar
    \brief  A classic ProgressBar object
*/
class ProgressBar : public UI_widget {
    public:
    ///\cond HIDDEN_SYMBOLS
    ProgressBar(UI_container *parent);
    ~ProgressBar();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    ProgressBar();
#endif
    // clang-format off
  /*! \fn  ProgressBar();
    \brief Creates a Progressbar.
    \par examples:
    \code
        local pb = Ui.ProgressBar.new()
        pb:set_value(20)
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
    \brief Sets max value of an progressbar object
    \param value int value. The max value of the progressbar object.
    \sa set_value()
    \sa set_min_value()
    \par examples:
    \code
        local pb = Ui.ProgressBar.new()
        pb:set_max_value(20) -- The max value
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
    \brief Sets the min value of an progressbar object
    \param value int value. The min value of the progressbar object.
    \sa set_value()
    \par examples:
    \code
        local pb = Ui.ProgressBar.new()
        pb:set_min_value(0) -- The minimal value
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
    \brief Sets value of an progressbar object shown to the user.
    \param value int value. The value of the progressbar object shown to the user.
    \sa set_value()
    \par examples:
    \code
        local pb = Ui.ProgressBar.new()
        pb:set_value(20) -- The value the user can see now
    \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    increment_value();
#endif
    /// \cond HIDDEN_SYMBOLS
    void increment_value();
    /// \endcond
    // clang-format off
  /*! \fn  increment_value();
        \brief Increments value of an progressbar by one.
        \sa set_value()
        \par examples:
        \code
            local pb = Ui.ProgressBar.new()
            pb:increment_value() -- The progress progressed by one

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
    \brief Sets the caption of an progressbar object.
    \param caption String value. The caption of the progressbar object.
    \details Caption is displayed as a title of the progressbar.
    \sa get_caption()
    \par examples:
    \code
        local pb = Ui.ProgressBar.new()
        pb:set_caption("TestEdit")
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
      \brief sets the visibility of the progressbar object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the progressbar object is hidden
          \li If \c true: the progressbar object is visible (default)
      \par examples:
      \code
        local pb = Ui.ProgressBar.new()
        pb:set_visible(false)   -- progressbar object is hidden
        pb:set_visible(true)   -- progressbar object is visible
      \endcode
  */
    // clang-format on

    private:
    QLabel *label = nullptr;
    QProgressBar *progressbar = nullptr;
};
/** \} */ // end of group ui

#endif // PROGRESSBAR_H
