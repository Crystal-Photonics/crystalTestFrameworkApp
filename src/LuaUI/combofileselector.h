#ifndef COMBOFILESELECTOR_H
#define COMBOFILESELECTOR_H
#include <QComboBox>
#include <QDateTime>
#include <QList>
#include <QMetaObject>
#include <QPushButton>
#include <functional>
#include <sol_forward.hpp>
#include <string>

#include "ui_container.h"
/// \cond HIDDEN_SYMBOLS
class FileEntry {
    public:
    QString filename;
    QString filenpath;
    QDateTime date;
};
/// \endcond

/** \ingroup ui
 *  \{
 */
// clang-format off
/*!
  \class   ComboBoxFileSelector
  \brief
    Interface to a ComboBoxFileSelector Ui-Element which the lua-script author can use
    to display a ComboBoxFileSelector to the script-user. It lists all files of a
    predefined directory which match the specified filter entries. The script-user
    can select a file which can be queried by the script using the function get_selected_file().
  \image html ComboBoxFileSelector.png ComboBoxFileSelector listing all found files and an explore button to open the directory
  \image latex ComboBoxFileSelector.png ComboBoxFileSelector listing all found files and an explore button to open the directory
  \image rtf ComboBoxFileSelector.png ComboBoxFileSelector listing all found files and an explore button to open the directory
  */
class ComboBoxFileSelector : public UI_widget {
public:



#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  ComboBoxFileSelector(string directory, string_table filter);
#endif
  /// \cond HIDDEN_SYMBOLS
  ComboBoxFileSelector(UI_container *parent, const std::string &directory,
                       const QStringList &filter);
  ~ComboBoxFileSelector();
  /// \endcond

  /*! \fn  ComboBoxFileSelector(string directory, string_table filter);
      \brief Creates a ComboBoxFileSelector object.
        \param directory specifies the directory in which the files are searched. If you use a relative path,
            the current directory is the directory in which the lua script is located
        \param filter is a table of strings defining filter. e.g {"*.lua"} lists only lua files.


      \par examples:
      \code
            local combobox_fileselector = Ui.ComboBoxFileSelector.new(".",{"*.lua","*.xml"})
            local done_button = Ui.Button.new("Select File")
            done_button:await_click()
            print("You selected: " ..file_selector:get_selected_file())
      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_order_by(string field, bool ascending);
#endif
  /// \cond HIDDEN_SYMBOLS
  void set_order_by(const std::string &field, const bool ascending);
  /// \endcond

  /*! \fn  set_order_by(string field, bool ascending)
      \brief Defines the order in which the files are listed in the combobox
        \param field defines which file- property is used for sorting. The following values are allowed:
          \li \c "name": sorts the files in alphabetical order.
          \li \c "date": sorts the files in chronological order.

        \param ascending: if true the files are sorted using ascending order


      \par examples:
      \code
            local combobox_fileselector = Ui.ComboBoxFileSelector.new(".",{"*.lua","*.xml"})
            local done_button = Ui.Button.new("Select File")
            file_selector:set_order_by("name",true) -- alphabetical, ascending order
            done_button:await_click()
            print("You selected: " ..file_selector:get_selected_file())
      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
   string get_selected_file();
#endif
  /// \cond HIDDEN_SYMBOLS
  std::string get_selected_file();
  /// \endcond

  /*! \fn  string get_selected_file();
      \brief Returns the absolute file path of the selected file.
        \returns the absolute file path of the selected file.


      \par examples:
      \code
            local combobox_fileselector = Ui.ComboBoxFileSelector.new(".",{"*.lua","*.xml"})
            local done_button = Ui.Button.new("Select File")
            done_button:await_click()
            print("You selected: " ..file_selector:get_selected_file())
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
      \brief sets the visibility of the ComboBoxFileSelector.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the comboboxfileselector is hidden
          \li If \c true: the comboboxfileselector is visible (default)

      \par examples:
      \code
            local combobox_fileselector = Ui.ComboBoxFileSelector.new(".",{"*.lua","*.xml"})
            combobox_fileselector:set_visible(false) --combobox_fileselector is hidden
      \endcode
  */

private:
  /// \cond HIDDEN_SYMBOLS
  void scan_directory();
  void fill_combobox();

  // UI_container *parent = nullptr;
  QComboBox *combobox = nullptr;
  QPushButton *button = nullptr;
  QList<FileEntry> file_entries;
  QStringList filters;
  QMetaObject::Connection button_clicked_connection;
  QString current_directory;
  /// \endcond
};
/** \} */ // end of group ui
#endif    // COMBOFILESELECTOR_H
