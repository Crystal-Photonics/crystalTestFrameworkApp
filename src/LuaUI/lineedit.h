#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QMetaObject>
#include <functional>
#include <string>

class QSplitter;
class QLineEdit;
/** \ingroup ui
 *  \{
 */
class LineEdit {
    public:
    ///\cond HIDDEN_SYMBOLS
    LineEdit(QSplitter *parent);
    ~LineEdit();
    ///\endcond
    void set_placeholder_text(const std::string &text); //!<\brief Puts a gray explaining text into the line edit.
                                                        //!< \param text the explaining text.
                                                        //!< \sa get_number()
                                                        //!< \par examples:
                                                        //!< \code
                                                        //!  	local le = Ui.LineEdit.new()
                                                        //!  	local stringvalue = le:get_text()
                                                        //!   print(stringvalue) -- prints text
                                                        //! \endcode

    std::string get_text() const; //!<\brief Returns the string value the user entered.
                                  //!< \return the text of the line edit as a string.
                                  //!< \sa get_number()
                                  //!< \par examples:
                                  //!< \code
                                  //!  	local le = Ui.LineEdit.new()
                                  //!  	local stringvalue = le:get_text()
                                  //!   print(stringvalue) -- prints text
                                  //! \endcode

    double get_number() const; //!<\brief Returns the string value the user entered converted to a number.
                               //!< \return the umber of line edits value.
                               //!< \details If it is not possible to convert the value to a number a messagebox
                               //! is shown asking the user to enter a number. This message box shows also the
                               //! name of the line edit telling the user which line edit has the issue and needs
                               //! a corrected value.
                               //!< \sa set_name()
                               //!< \par examples:
                               //!< \code
                               //!  	local le = Ui.LineEdit.new()
                               //!  	le:set_name("TestEdit")
                               //!  	local num = le:get_number() -- if number invalid (eg. empty text field)
                               //!                                 -- a message dialog is shown with a text like
                               //!                                 -- "Text of $name is not a number please enter a number"
                               //!     print(num) -- prints number
                               //! \endcode

    void set_text(const std::string &text); //!<\brief Sets text of an line edit object.
                                            //!< \param text String value. The text of the line edit object shown to the user.
                                            //! \sa get_text()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local le = Ui.LineEdit.new()
                                            //!  	le:set_text("TestEdit") -- The text the user can edit now
                                            //!                             -- is preset to "TestEdit".
                                            //! \endcode

    void set_name(const std::string &name); //!<\brief Sets the name of an line edit object.
                                            //!< \param name String value. The name of the line edit object.
                                            //!< \details Is used in the error message of get_number() to clearify where
                                            //! the string to number conversion issue comes from.
                                            //! \sa get_number()
                                            //! \sa get_name()
                                            //!< \par examples:
                                            //!< \code
                                            //!  	local le = Ui.LineEdit.new()
                                            //!  	le:set_name("TestEdit")
                                            //!  	local num = le:get_number() -- if number invalid (eg. empty text field)
                                            //!                                 -- a message dialog is shown with a text like
                                            //!                                 -- "Text of $name is not a number please enter a number"
                                            //!     print(num) -- prints number
                                            //! \endcode

    std::string get_name() const; //!<\brief Returns the name of an line edit object.
                                  //!< \return the name of the line edit object set by set_name() as a string value.
                                  //! \sa get_number()
                                  //! \sa set_name()

///\endcond
//! \brief Waits until user hits the return key.
//! \par examples:
//! \code
//! local le = Ui.LineEdit.new()
//! le:await_return() -- waits until user hits return key
//! \endcode
#ifdef DOXYGEN_ONLY
    //this block is just for ducumentation purpose
    await_return();
#endif
    ///\cond HIDDEN_SYMBOLS
    void set_single_shot_return_pressed_callback(std::function<void()> callback);
    ///\endcond
    private:
    QLineEdit *edit = nullptr;
    std::string name;
    QMetaObject::Connection callback_connection = {};
};
/** \} */ // end of group ui
#endif // LINEEDIT_H
