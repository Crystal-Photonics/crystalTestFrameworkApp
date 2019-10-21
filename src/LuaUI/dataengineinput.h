#ifndef DATAENGINEINPUT_H
#define DATAENGINEINPUT_H

#include "data_engine/data_engine.h"
#include "scriptengine.h"
#include "ui_container.h"
#include <QMetaObject>
#include <functional>
#include <string>

class QLabel;
class QSplitter;
class QTimer;
class QCheckBox;
class QHBoxLayout;
class QWidget;
class QLineEdit;
class QPushButton;

/** \ingroup ui
 *  \{
 */

// clang-format off
/*!
  \class   DataEngineInput
  \author Tobias Rieger (tr@crystal-photonics.com),<br> Arne Krüger
  (ak@crystal-photonics.com)
  \brief DataEngineInput Ui-Element
  \par
    Interface to a DataEngineInput object which the lua-script author can use
    to display a DataEngineInput object to the script-user. It is used to interact
    with a specified field of the DataEngine. It can be used to let the user manually
    enter actual values and to display whether the actual value is within the tolerances
    of the desired value.
  */

class DataEngineInput : public UI_widget {
  ///\cond HIDDEN_SYMBOLS
  enum FieldType { Bool, String, Numeric };
  ///\endcond

public:
  ///\cond HIDDEN_SYMBOLS

  ///\endcond

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  DataEngineInput(
        Data_engine data_engine,
        string data_engine_field,
        string extra_explanation,
        string empty_value_placeholder,
        string desired_prefix,
        string actual_prefix);
#endif
  /// \cond HIDDEN_SYMBOLS
  DataEngineInput(UI_container *parent_, ScriptEngine *script_engine,
                  Data_engine *data_engine_, std::string field_id_,
                  std::string extra_explanation,
                  std::string empty_value_placeholder,
                  std::string desired_prefix, std::string actual_prefix_);
  ~DataEngineInput();
  /// \endcond

  /*! \fn    DataEngineInput(Data_engine data_engine,string data_engine_field,
                string extra_explanation,string empty_value_placeholder,
                string desired_prefix,string actual_prefix);
      \brief Creates a DataEngineInput object.
        \param data_engine          specifies the Data_engine object in which the desired and actual values are stored.
        \param data_engine_field    specifies the field of the Data_engine object which is displayed to the user
        \param extra_explanation    displays an extra explanatory text close to the field which can be helpful for the user
        \param empty_value_placeholder specifies which string should be displayed in case the field is empty. eg "/"
        \param desired_prefix       specifies the prefix for the desired value. e.g. "Desired: "
        \param actual_prefix        specifies the prefix for the actual value. e.g. "Actual: " or "Measured: "

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")


            sleep_ms(1000)
            data_engine:set_actual_number("gerate_daten/max_current_1",5)
            data_engine_input_number:load_actual_value()

            local data_engine_input_bool = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/bool_test1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Correct?:")
            data_engine_input_bool:await_event()
            local data_engine_input_text = Ui.DataEngineInput.new(data_engine,
                                                                "messmittel/multimeter_hersteller",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Actual:")
            data_engine_input_text:await_event()
      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  load_actual_value();
#endif
  /// \cond HIDDEN_SYMBOLS
  void load_actual_value();
  /// \endcond

  /*! \fn    load_actual_value();
      \brief Updates the displayed actual value from the Data_engine object.


      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")


            sleep_ms(1000)
            data_engine:set_actual_number("gerate_daten/max_current_1",5)
            data_engine_input_number:load_actual_value()

      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  save_to_data_engine();
#endif
  /// \cond HIDDEN_SYMBOLS
  void save_to_data_engine();
  /// \endcond

  /*! \fn    save_to_data_engine();
      \brief if the DataEngineInput object is editable the user enters the actual value manually.
            Use this method to store the entered value into the Data_engine object.
            This function is similar to DataEngineInput::await_event() which also writes the
            actual value to the data_engine object when the user clicks the buttons "yes", "no" or "next".

      \sa  DataEngineInput::await_event()

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")

            data_engine_input_number:set_editable()
            sleep_ms(5000)
            data_engine_input_number:save_to_data_engine()

      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  await_event();
#endif
  /// \cond HIDDEN_SYMBOLS
  void await_event();
  /// \endcond

  /*! \fn    await_event();
      \brief Waits until the user clicked any button of the DataEngineInput object. If it is connected
        to a Data_engine bool field it waits until the user clicks the Buttons "Yes", "No" or "Exceptional approval".
        Else it waits until the buttons "Next" or "Exceptional approval" are clicked.
        If the DataEngineInput object is editable it stores the actual value into the Data_engine field.

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/bool_test1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")

            data_engine_input_number:await_event()
            print("the actual value of gerate_daten/bool_test1 is: "..tostring(data_engine:get_actual_value("gerate_daten/bool_test1")))

      \endcode
  */


#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  sleep_ms(int timeout_ms);
#endif
  /// \cond HIDDEN_SYMBOLS
  void sleep_ms(uint timeout_ms);
  /// \endcond

  /*! \fn    sleep_ms(int timeout_ms);
      \brief waits until timeout has passed.
        \param timeout_ms timeout in milliseconds

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/bool_test1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")

            data_engine_input_number:sleep_ms(2000)
            print("the actual value of gerate_daten/bool_test1 is: "..tostring(data_engine:get_actual_value("gerate_daten/bool_test1")))

      \endcode
  */


#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_explanation_text(string extra_explanation);
#endif
  /// \cond HIDDEN_SYMBOLS
  void set_explanation_text(const std::string &extra_explanation);
  /// \endcond

  /*! \fn    set_explanation_text(string extra_explanation);
      \brief Overwrites the extra explanatory help text for the user
        \param extra_explanation the explanatory text.

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/bool_test1",
                                                                "press any key foo",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")

            data_engine_input_number:set_explanation_text("press any key bar")
      \endcode
  */

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_editable();
#endif
  /// \cond HIDDEN_SYMBOLS
  void set_editable();
  /// \endcond

  /*! \fn    set_editable();
      \brief Sets the DataEngineInput object into edit mode. In this mode the user has access
            to the actual value of the Data_engine field. If it is an Number or Textfield etc
            a text input field is displayed. If it is an boolean value the buttons "yes" and "no" are
            displayed.


      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")

            data_engine_input_number:set_editable()
            sleep_ms(5000)
            data_engine_input_number:save_to_data_engine()
      \endcode
  */



  bool get_is_editable();
  /*! \fn   bool get_is_editable();
      \brief Returns whether the DataEngineInput object is in edit mode or not.
      \returns whether the DataEngineInput object is in edit mode or not.


      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")
            print(data_engine_input_number:get_is_editable()) --prints "false"
            data_engine_input_number:set_editable()
            sleep_ms(5000)
            data_engine_input_number:save_to_data_engine()
            print(data_engine_input_number:get_is_editable()) --prints "true"
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
      \brief sets the visibility of the DataEngineInput object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the DataEngineInput object is hidden
          \li If \c true: the DataEngineInput object is visible (default)

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")
          data_engine_input_number:set_visible(false)   -- data_engine_input_number is disabled
      \endcode
  */


#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  set_enabled(bool is_enabled);
#endif
  /// @cond HIDDEN_SYMBOLS
  void set_enabled(bool is_enabled);
  /// @endcond
  // clang-format off
  /*! \fn  set_enabled(bool is_enabled);
      \brief sets the enable state of the DataEngineInput object.
      \param is_enabled whether enabled or not. (true / false)
          \li If \c false: the DataEngineInput object is disabled and gray
          \li If \c true: the DataEngineInput object enabled (default)

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)
            local data_engine_input_number = Ui.DataEngineInput.new(data_engine,
                                                                "gerate_daten/max_current_1",
                                                                "press any key",
                                                                "empty",
                                                                "Desired:",
                                                                "Measured:")
          data_engine_input_number:set_enabled(false)   -- data_engine_input_number is disabled
          data_engine_input_number:set_enabled(true)   -- data_engine_input_number is enabled
      \endcode
  */
  ///\cond HIDDEN_SYMBOLS
  // clang-format on
private:
  QLabel *label_extra_explanation = nullptr;
  QLabel *label_de_description = nullptr;
  QLabel *label_de_desired_value = nullptr;
  QLabel *label_de_actual_value = nullptr;
  QLabel *label_ok = nullptr;

  QPushButton *button_yes = nullptr;
  QLabel *label_yes = nullptr;

  QPushButton *button_no = nullptr;
  QLabel *label_no = nullptr;

  QPushButton *button_next = nullptr;
  QLabel *label_next = nullptr;

  QPushButton *button_exceptional_approval = nullptr;
  QLabel *label_exceptional_approval = nullptr;

  QLineEdit *lineedit = nullptr;
  QTimer *timer = nullptr;

  Data_engine *data_engine = nullptr;
  QHBoxLayout *hlayout = nullptr;
  QString field_id;

  QString empty_value_placeholder;
  QString extra_explanation;
  QString desired_prefix;
  QString actual_prefix;
  uint blink_state = 0;
  bool is_editable = false;

  bool is_enabled = true;
  bool is_visible = true;
  bool is_waiting = false;

  void start_timer();

  ScriptEngine *script_engine;
  void resizeMe(QResizeEvent *event) override;
  std::experimental::optional<bool> bool_result;

  QMetaObject::Connection callback_timer = {};
  QMetaObject::Connection callback_bool_yes = {};
  QMetaObject::Connection callback_bool_no = {};
  QMetaObject::Connection callback_next = {};
  QMetaObject::Connection callback_exceptional_approval = {};
  void set_ui_visibility();
  void set_button_visibility(bool next, bool yes_no);
  FieldType field_type;
  void set_labels_enabled();
  void set_total_visibilty();
  const int BLINK_INTERVAL_MS = 500;
  uint total_width = 0;
  bool init_ok = false;

  bool dont_save_result_to_de = false;
  void scale_columns();
  ///\endcond
};
/** \} */ // end of group ui
#endif    // DATAENGINEINPUT_H
