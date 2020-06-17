#ifndef LINEEDIT_H
#define LINEEDIT_H

#include "scriptengine.h"
#include "ui_container.h"
#include <QDateEdit>
#include <QMetaObject>
#include <functional>
#include <string>

class TestScriptEngine;
class QLabel;
class QSplitter;
class QLineEdit;
class QWidget;

/** \ingroup ui
 *  \{
 */
// clang-format off
/*!
  \class   LineEdit
  \brief LineEdit Ui-Element. It enables the user to enter text or numbers.
  \image html LineEdit.png Simple LineEdit for user input
  \image latex LineEdit.png Simple LineEdit for user input
  \image rtf LineEdit.png Simple LineEdit for user input
  \image html LineEdit2.png LineEdit with label and live input validation
  \image latex LineEdit2.png LineEdit with label and live input validation
  \image rtf LineEdit2.png LineEdit with label and live input validation
  */
// clang-format on

///\cond HIDDEN_SYMBOLS
enum class LineEdit_Entermode { TextMode, DateMode };

class PatternCheck {
    public:
    enum { None, yyww_m, yyww } t = PatternCheck::None;

    PatternCheck(decltype(t) me)
        : t{me} {}

    PatternCheck(QString str) {
        if (str == "") {
            t = PatternCheck::None;
        } else if (str == "YY/WW?-") {
            t = PatternCheck::yyww_m;
        } else if (str == "YY/WW?") {
            t = PatternCheck::yyww;
        } else {
            throw std::runtime_error(
                QObject::tr("The pattern %1 is not valid. Allowed patterns: %2").arg(str).arg(allowed_pattern_names().join("\n")).toStdString());
        }
    }

    QString to_string() const {
        switch (t) {
            case PatternCheck::None:
                return "";
            case PatternCheck::yyww_m:
                return "YY/WW?-";
            case PatternCheck::yyww:
                return "YY/WW?";
        }
        return "";
    }

    static QStringList allowed_pattern_names() {
        return QStringList{"\"YY/WW?-\"", "\"YY/WW?\"", "\"\""};
    }

    QStringList example_matching_to_pattern() const {
        switch (t) {
            case yyww_m: {
                return QStringList{
                    "YY/WW?- means the first 2 characters of the year > 17\nseperated by a '/' followed be the weeknumber and an optional character.\nThe "
                    "date must not be a future date. It may be left empty with an '-'. Example:\n'18/33', '18/33a', '-'"};
            }
            case yyww: {
                return QStringList{
                    "YY/WW? means the first 2 characters of the year > 17\nseperated by a '/' followed be the weeknumber and an optional character.\nThe "
                    "date must not be a future date. Example:\n'18/33', '18/33a'"};
            }
            case None:
                return QStringList{};
        }
        return QStringList{};
    }

    bool is_input_matching_to_pattern(const QString &string_under_test) const;
};

///\endcond

class LineEdit : public UI_widget {
    friend TestScriptEngine;

    public:
    ///\cond HIDDEN_SYMBOLS
    LineEdit(UI_container *parent, ScriptEngine *script_engine);
    ~LineEdit();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    LineEdit();
#endif
    // clang-format off
    /*! \fn  LineEdit(string text);
    \brief Creates a LineEdit object.
    \par examples:
    \code
        local le = Ui.LineEdit.new()
    \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_date_mode();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_date_mode();
#endif
    // clang-format off
    /*! \fn  set_date_mode();
    \brief switches the LineEdit into datemode.
    \sa set_text_mode()
    \par examples:
    \code
        local le = Ui.LineEdit.new()
        local stringvalue = le:set_date_mode() --le is in date mode now
    \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_text_mode();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_text_mode();
#endif
    // clang-format off
    /*! \fn  set_text_mode();
    \brief switches the LineEdit into textmode which is its default mode.
    \sa set_date_mode()
    \par examples:
    \code
        local le = Ui.LineEdit.new()
        local stringvalue = le:set_text_mode() --le is in text mode now
    \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_placeholder_text(const std::string &text);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_placeholder_text(string text);
#endif
    // clang-format off
    /*! \fn  set_placeholder_text(string text);
    \brief Puts a gray explaining text into the line edit.
    \param text the explaining text.
    \sa get_number()
    \par examples:
    \code
        local le = Ui.LineEdit.new()
        local stringvalue = le:get_text()
      print(stringvalue) -- prints text
    \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_input_check(const std::string &text);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_input_check(string text);
#endif
    // clang-format off
    /*! \fn  set_input_check(string text);
    \brief Enables an input check. Only input is accepted which matches the pattern. Valid patterns are "YY/WW?-", "YY/WW?" and "".
    \par examples:
    \code
        local le = Ui.LineEdit.new()
        le:set_input_check("YY/WW?")
            --where the first 2 digits(YY) are the year and the second(WW) is the week
            --it will reject numbers in future and numbers before 2018

            --only numbers like 18/33a or 18/33 are ok
            --numbers like "17/33a", "33/18", "1833", "" or "-" will be rejected

        le:set_input_check("YY/WW?-")
            -- as "YY/WW?" except that now also "-" is allowed as input
    \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    std::string get_text();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_text();
#endif
    // clang-format off
    /*! \fn  string get_text();
    \brief Returns the string value the user entered.
    \return the text of the line edit as a string.
    \sa get_number()
    \par examples:
    \code
        local le = Ui.LineEdit.new()
        local stringvalue = le:get_text()
      print(stringvalue) -- prints text
    \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    double get_date() const;
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    double get_date();
#endif
    // clang-format off
    /*! \fn  double get_date()
        \brief Returns the date value the user entered.
        \return the date of the line edit as a seconds since epoch.
        \sa get_number()
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            local date_value = le:get_date()
          print(os.date("%d.%m.%Y",date_value)) -- prints text: example: 06.10.2012
        \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    double get_number() const;
///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    double get_number();
#endif
    // clang-format off
    /*! \fn  double get_number();
        \brief Returns the string value the user entered converted to a number.
        \return The number represented by the LineEdit's text.
        \details If it is not possible to convert the text to a number, a messagebox
        is shown asking the user to enter a number. This message box shows also the
        name of the line edit telling the user which line edit has the issue and needs
        a corrected value.
        \sa set_name()
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            le:set_name("TestEdit")
            local num = le:get_number() -- if number invalid (eg. empty text field)
                                        -- a message dialog is shown with a text like
                                        -- "Text of $name is not a number please enter a number"
            print(num) -- prints number
        \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_text(const std::string &text);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_text(string text);
#endif
    // clang-format off
    /*! \fn  set_text(string text);
        \brief Sets text of an line edit object.
        \param text String value. The text of the line edit object shown to the user.
        \sa get_text()
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            le:set_text("TestEdit") -- The text the user can edit now
                                    -- is preset to "TestEdit".
        \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_date(double date_value_since_epoch);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_date(double date_value_since_epoch);
#endif
    // clang-format off
    /*! \fn  set_date(double date_value_since_epoch);
        \brief Sets date of an line edit object. If it is not yet in date mode, it switches to datemode now.
        \param date_value_since_epoch value. The date which will be displayed in date mode. number of seconds since epoch(aka unixdate)
        \sa set_date_mode()
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            le:set_date(os.time()) -- The date the user can edit now
                                    -- is preset to "TestEdit".
        \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_name(const std::string &name);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_name(string name);
#endif
    // clang-format off
    /*! \fn  set_name(string name);
        \brief Sets the name of an line edit object.
        \param name String value. The name of the line edit object.
        \details Is used in the error message of get_number() to clearify where
        the string to number conversion issue comes from.
        \sa get_number()
        \sa get_name()
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            le:set_name("TestEdit")
            local num = le:get_number() -- if number invalid (eg. empty text field)
                                        -- a message dialog is shown with a text like
                                        -- "Text of $name is not a number please enter a number"
            print(num) -- prints number
        \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    std::string get_name() const;
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_name();
#endif
    // clang-format off
    /*! \fn  string get_name();
        \brief Returns the name of an line edit object.
        \return the name of the line edit object set by set_name() as a string value.
        \sa get_number()
        \sa set_name()
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_caption(const std::string &caption);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_caption(string caption);
#endif
    // clang-format off
    /*! \fn  set_caption(string caption);
        \brief Sets the caption of an line edit object.
        \param caption String value. The caption of the line edit object.
        \details Caption is displayed as a title of the line edit.
        \sa get_caption()
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            le:set_caption("TestEdit")
        \endcode
    */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    std::string get_caption() const;
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_caption();
#endif
    // clang-format off
    /*! \fn  string get_caption();
        \brief Returns the caption.
        \return the caption of the line edit object set by set_caption() as a string value.
        \sa set_caption()
    */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_focus();
#endif
    /// @cond HIDDEN_SYMBOLS
    void set_focus();
    /// @endcond
    // clang-format off
  /*! \fn  set_focus();
      \brief sets the focus to the line edit object. This has the effect that the cursor moves into the line edit object.

      \par examples:
      \code
          local le = Ui.LineEdit.new()
          le:set_visible(false)   -- line edit object is hidden
          le:set_visible(true)   -- line edit object is visible
      \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void await_return();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    await_return();
#endif
    // clang-format off
    /*! \fn  await_return();
        \brief Waits until user hits the return key.
        \par examples:
        \code
            local le = Ui.LineEdit.new()
            le:await_return() -- waits until user hits return key
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
      \brief sets the visibility of the line edit object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the line edit object is hidden
          \li If \c true: the line edit object is visible (default)
      \sa LineEdit::set_enabled()
      \par examples:
      \code
          local le = Ui.LineEdit.new()
          le:set_visible(false)   -- line edit object is hidden
          le:set_visible(true)   -- line edit object is visible
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
      \brief sets the enable state of the line edit object.
      \param enable whether enabled or not. (true / false)
          \li If \c false: the line edit object is disabled and gray
          \li If \c true: the line edit object enabled (default)
      \sa LineEdit::set_visible()
      \par examples:
      \code
          local le = Ui.LineEdit.new()
          le:set_enabled(false)   -- line edit object is disabled
          le:set_enabled(true)   -- line edit object is enabled
      \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void load_from_cache(void);
    void save_to_cache();
    ///\endcond
    private:
    ///\cond HIDDEN_SYMBOLS
    QLabel *label = nullptr;
    QLineEdit *text_edit = nullptr;
    QDateEdit *date_edit = nullptr;

    LineEdit_Entermode entermode;
    std::string name_m;
    std::string caption_m;
    ScriptEngine *script_engine;
    PatternCheck pattern_check_m;
    QMetaObject::Connection callback_text_changed = {};
    bool is_input_matching_to_pattern() const;

    const QString date_formatstring = "dd.MM.yyyy"; //20.07.1969
    QStringList example_matching_to_pattern(const QString &pattern_check) const;
    ///\endcond
};
/** \} */ // end of group ui
#endif    // LINEEDIT_H
