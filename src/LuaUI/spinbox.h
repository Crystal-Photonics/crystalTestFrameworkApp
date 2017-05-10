#ifndef SPINEDIT_H
#define SPINEDIT_H

#include <string>

class QLabel;
class QSplitter;
class QLineEdit;
class UI_container;
class QWidget;
class QSpinBox;

/** \ingroup ui
 *  \{
 */
class SpinBox {
    public:
    ///\cond HIDDEN_SYMBOLS
    SpinBox(UI_container *parent);
    ~SpinBox();
    ///\endcond


    int get_value() const; //!<\brief Returns the integer value the user entered.
                                  //!< \return the value of the spin box.
                                  //!< \par examples:
                                  //!< \code
                                  //!  	local sb = Ui.SpinBox.new()
                                  //!  	local intvalue = sb:get_value()
                                  //!   print(intvalue) -- prints value
                                  //! \endcode


    void set_max_value(const int value); //!<\brief Sets max value of an spin box object the user can select
                                            //!< \param value int value. The max value of the spin box object.
                                            //! \sa set_value()
                                            //! \sa set_min_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local sb = Ui.SpinBox.new()
                                            //!  	sb:set_max_value(20) -- The max value
                                            //!
                                            //! \endcode
                                            //!

    void set_min_value(const int value); //!<\brief Sets the min value of an spin box object the user can select.
                                            //!< \param value int value. The min value of the spin box object.
                                            //! \sa set_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local sb = Ui.SpinBox.new()
                                            //!  	sb:set_min_value(1) -- The minimal value
                                            //!
                                            //! \endcode
                                            //!

    void set_value(const int value); //!<\brief Sets value of an spin box object.
                                            //!< \param value int value. The value of the spin box object shown to the user.
                                            //! \sa set_value()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local sb = Ui.SpinBox.new()
                                            //!  	sb:set_value(20) -- The value the user can edit now
                                            //!
                                            //! \endcode



    void set_caption(const std::string &caption); //!<\brief Sets the caption of an spin box object.
                                            //!< \param caption String value. The caption of the spin box object.
                                            //!< \details Caption is displayed as a title of the spin box.
                                            //! \sa get_caption()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local sb = Ui.SpinBox.new()
                                            //!  	sb:set_caption("TestEdit")
                                            //! \endcode

    std::string get_caption() const; //!<\brief Returns the caption.
                                  //!< \return the caption of the spin box object set by set_caption() as a string value.
                                  //! \sa set_caption()


    private:
    QLabel *label = nullptr;
    QSpinBox *spinbox = nullptr;
};
/** \} */ // end of group ui

#endif // SPINEDIT_H
