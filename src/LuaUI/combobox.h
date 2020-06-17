#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>
#include <QLabel>
#include <sol_forward.hpp>

#include "ui_container.h"

/** \ingroup ui
 *  \{
 */

// clang-format off
/*!
  \class   ComboBox
  \brief
    A ComboBox Ui-Element to let a script-user choose out of a list of options.
  \image html ComboBox.png ComboBox closed
  \image html ComboBox_open.png ComboBox open
  \image latex ComboBox.png ComboBox closed
  \image latex ComboBox_open.png ComboBox open
  \image rtf ComboBox.png ComboBox closed
  \image rtf ComboBox_open.png ComboBox open
  */

class ComboBox : public UI_widget {
public:
#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  ComboBox(string_table items);
#endif
  ///\cond HIDDEN_SYMBOLS
  ComboBox(UI_container *parent, const sol::table &items);
  ~ComboBox();
  ///\endcond

  /*!   \fn ComboBox(string_table items);
        \brief Creates a Combobox
        \param items Table of strings which are listed in the ComboBox
        \par examples:
        \code
            local combobox = Ui.ComboBox.new({"Hello", "World","Foo"})
        \endcode
*/

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_items(string_table items);
#endif
  ///\cond HIDDEN_SYMBOLS
  void set_items(const sol::table &items);
  ///\endcond
  /*!   \fn set_items(string_table items)
          \brief Overwrites the list of the strings listed in the ComboBox
          \param items Table of strings which are listed in the ComboBox
          \par examples:
          \code
                local combobox = Ui.ComboBox.new({"Hello", "World","Foo"})
                local new_list = {"Hallo", "Welt","Bar"}
                combobox:set_items(new_list)
          \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  string get_text();
#endif
  ///\cond HIDDEN_SYMBOLS
  std::string get_text() const;
  ///\endcond
  /*! \fn string get_text()
      \brief returns the text selected in the Combobox
      \return the text selected in the ComboBox
      \par examples:
      \code
          local combobox = Ui.ComboBox.new({"Hello", "World","Foo"})
          local new_list = {"Hallo", "Welt","Bar"}
          combobox:set_items(new_list)
          local text = combobox:get_text()
          print(text)
      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_index(int index);
#endif
  ///\cond HIDDEN_SYMBOLS
  void set_index(unsigned int index);
  ///\endcond
  /*! \fn set_index(int index)
    \brief selects an item of the combobox list
    \param index the index of the item which will be selected. The first index is 1.
    \par examples:
    \code
        local combobox = Ui.ComboBox.new({"Hello", "World","Foo"})
        combobox:set_index(2) -- selects "World"
        local text = combobox:get_text()
        print(text)           -- prints "World"
    \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  int get_index();
#endif
  ///\cond HIDDEN_SYMBOLS
  unsigned int get_index();// return == 0 means: "no item is selected"
  ///\endcond
  /*! \fn get_index();
    \return returns the index of the selected element. If it returns 0, no item is selected. The first element is 1.
    \par examples:
    \code
        local combobox = Ui.ComboBox.new({"Hello", "World","Foo"})
        combobox:set_index(2) -- selects "World"
        local index = combobox:get_index()
        print(index)           -- prints 2
    \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_caption(string caption);
#endif
  ///\cond HIDDEN_SYMBOLS
  void set_caption(const std::string caption);
  ///\endcond
  /*! \fn set_caption(string caption)
    \brief sets the caption of the combobox. It is displayed above the box.

    \param caption the caption-text of the combobox.
    \par examples:
    \code
        local combobox = Ui.ComboBox.new({"blue", "red","black"})
        combobox:set_caption("please select color")
    \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  string get_caption();
#endif
  ///\cond HIDDEN_SYMBOLS
  std::string get_caption() const;
  ///\endcond
  /*! \fn get_caption();
    \returns the caption of the combobox
    \par examples:
    \code
        local combobox = Ui.ComboBox.new({"Hello", "World","Foo"})
        combobox:set_caption("Bar") -- adds label "Bar" above the combobox
        local caption = combobox:get_caption()
        print(caption)           -- prints "Bar"
    \endcode
  */



#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_name(string name);
#endif
  ///\cond HIDDEN_SYMBOLS
  void set_name(const std::string name);
  ///\endcond
  /*! \fn set_name(string name)
    \brief sets the name of the combobox. The name is used in error messages or in future for pre filling out forms.

    \param name the name of the combobox.
    \par examples:
    \code
        local combobox = Ui.ComboBox.new({"blue", "red","black"})
        combobox:set_name("product_color")
    \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_editable(bool visible);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_editable(bool editable);
  /// @endcond

  /*! \fn  set_editable(bool editable);
      \brief defines whether the text-field of the combobox can be used freely or it is restricted to the item-list of the combobox
      \param editable whether text-field can be used freely or not
          \li If \c false: the text-field is restricted to the strings of the  combobox's item list(default)
          \li If \c true: the text field can be used freely.

      \par examples:
      \code
          local combobox = Ui.ComboBox.new({"blue", "red","black"})
          combobox:set_editable(false)   -- combobox's text field can only contain the strings "blue", "red" and "black"
          combobox:set_editable(true)   -- combobox's text field can be edited freely by the user
      \endcode
  */


#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_visible(bool visible);
#endif
  /// \cond HIDDEN_SYMBOLS
  void set_visible(bool visible);
  /// \endcond

  /*! \fn  set_visible(bool visible);
      \brief sets the visibility of the combobox.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the combobox is hidden
          \li If \c true: the combobox is visible (default)

      \par examples:
      \code
          local combobox = Ui.ComboBox.new({"blue", "red","black"})
          combobox:set_visible(false)   -- combobox is hidden
          combobox:set_visible(true)   -- combobox is visible
      \endcode
  */


private:
  QString name_m;
  QComboBox *combobox = nullptr;
  QLabel *label = nullptr;
};
/** \} */ // end of group ui
#endif    // COMBOBOX_H
