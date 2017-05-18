#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H



#include <string>

class QLabel;
class QSplitter;
class QLineEdit;
class UI_container;
class QWidget;
class QProgressBar;

/** \ingroup ui
 *  \{
 */
class ProgressBar {
    public:
    ///\cond HIDDEN_SYMBOLS
    ProgressBar(UI_container *parent);
    ~ProgressBar();
    ///\endcond




    void set_max_value(const int value); //!<\brief Sets max value of an progressbar object
                                            //!< \param value int value. The max value of the progressbar object.
                                            //! \sa set_value()
                                            //! \sa set_min_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local pb = Ui.ProgressBar.new()
                                            //!  	pb:set_max_value(20) -- The max value
                                            //!
                                            //! \endcode
                                            //!

    void set_min_value(const int value); //!<\brief Sets the min value of an progressbar object
                                            //!< \param value int value. The min value of the progressbar object.
                                            //! \sa set_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local pb = Ui.ProgressBar.new()
                                            //!  	pb:set_min_value(0) -- The minimal value
                                            //!
                                            //! \endcode
                                            //!

    void set_value(const int value); //!<\brief Sets value of an progressbar object shown to the user.
                                            //!< \param value int value. The value of the progressbar object shown to the user.
                                            //! \sa set_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local pb = Ui.ProgressBar.new()
                                            //!  	pb:set_value(20) -- The value the user can see now
                                            //!
                                            //! \endcode

    void increment_value();                 //!<\brief Increments value of an progressbar by one.
                                            //! \sa set_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local pb = Ui.ProgressBar.new()
                                            //!  	pb:increment_value() -- The progress progressed by one
                                            //!
                                            //! \endcode


    void set_caption(const std::string &caption); //!<\brief Sets the caption of an progressbar object.
                                            //!< \param caption String value. The caption of the progressbar object.
                                            //!< \details Caption is displayed as a title of the progressbar.
                                            //! \sa get_caption()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local pb = Ui.ProgressBar.new()
                                            //!  	pb:set_caption("TestEdit")
                                            //! \endcode

    std::string get_caption() const; //!<\brief Returns the caption.
                                  //!< \return the caption of the spin box object set by set_caption() as a string value.
                                  //! \sa set_caption()

    void set_visible(bool visible);

    private:
    QLabel *label = nullptr;
    QProgressBar *progressbar = nullptr;
};
/** \} */ // end of group ui


#endif // PROGRESSBAR_H
