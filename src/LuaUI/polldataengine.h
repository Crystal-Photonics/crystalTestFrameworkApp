#ifndef POLLDATAENGINE_H
#define POLLDATAENGINE_H

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
class QGridLayout;
class QWidget;
class QLineEdit;
class QPushButton;

/** \ingroup ui
 *  \{
 */

/*!
  \class   PollDataEngine
  \brief
    A PollDataEngine object can be used to display the current state of various fields of the data engine.
    This way the user can be kept updated e.g. whether there are problems with the measurements.
  */

///\cond HIDDEN_SYMBOLS

class FieldEntry {
    public:
    enum FieldType { Bool, String, Numeric };
    QString field_id;
    QLabel *label_de_description = nullptr;
    QLabel *label_de_desired_value = nullptr;
    QLabel *label_de_actual_value = nullptr;
    QLabel *label_ok = nullptr;
    FieldType field_type;
};
///\endcond

class PollDataEngine : public UI_widget {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    PollDataEngine(Data_engine data_engine, string_table items);
#endif
    /// @cond HIDDEN_SYMBOLS
    PollDataEngine(UI_container *parent_, Data_engine *data_engine_, QStringList items);
    ~PollDataEngine();
    /// @endcond
    // clang-format off
  /*! \fn  PollDataEngine(Data_engine data_engine, string_table items);
      \brief Creates a PollDataEngine object.
      \param data_engine specifies the Data_engine object in which the desired and actual values are stored.
      \param items specifies a list of fields of the Data_engine object which are displayed to the user

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)

            local poll = Ui.PollDataEngine.new(data_engine,{"gerate_daten/max_current_1","gerate_daten/bool_test1"})

      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    refresh();
#endif
    /// @cond HIDDEN_SYMBOLS
    void refresh();
    /// @endcond
    // clang-format off
  /*! \fn  refresh();
      \brief Refreshes PollDataEngine object and reloads the actual values of the dataengine.


      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)

            local poll = Ui.PollDataEngine.new(data_engine,{"gerate_daten/max_current_1","gerate_daten/bool_test1"})
            data_engine:set_actual_number("gerate_daten/max_current_1",5)
            poll:refresh()
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    bool is_in_range();
#endif
    /// @cond HIDDEN_SYMBOLS
    bool is_in_range();
    /// @endcond
    // clang-format off
  /*! \fn  bool is_in_range();
      \brief Returns True if all actual values of the data engine fields mentioned in PollDataEngine::PollDataEngine() are in range.
      \returns  True if all actual values of the data engine fields mentioned in PollDataEngine::PollDataEngine() are in range.

      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)

            local poll = Ui.PollDataEngine.new(data_engine,{"gerate_daten/max_current_1","gerate_daten/bool_test1"})
            local in_range = poll:is_in_range()
            print(in_range)
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
      \brief sets the visibility of the PollDataEngine object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the PollDataEngine object is hidden
          \li If \c true: the PollDataEngine object is visible (default)
      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)

            local poll = Ui.PollDataEngine.new(data_engine,{"gerate_daten/max_current_1","gerate_daten/bool_test1"})
            poll:set_visible(false)   -- PollDataEngine object is hidden
            poll:set_visible(true)   -- PollDataEngine object is visible
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_enabled(bool is_enabled);
#endif
    /// @cond HIDDEN_SYMBOLS
    void set_enabled(bool is_enabled);
    /// @endcond
    // clang-format off
  /*! \fn  set_enabled(bool enable);
      \brief sets the enable state of the PollDataEngine object.
      \param enable whether enabled or not. (true / false)
          \li If \c false: the PollDataEngine object is disabled and gray
          \li If \c true: the PollDataEngine object enabled (default)
      \par examples:
      \code
            local dependency_tags = {}
            local data_engine = Data_engine.new(
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\report_template.lrxml",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\desire_values_aka_data_engine_source.json",
                EXAMPLE_PATH.."\\data_engine_and_report\\1\\dump\\test\\",
                dependency_tags)

            local poll = Ui.PollDataEngine.new(data_engine,{"gerate_daten/max_current_1","gerate_daten/bool_test1"})
            label:set_enabled(false)   --  PollDataEngine object is disabled
            label:set_enabled(true)   --  PollDataEngine object is enabled
      \endcode
  */
    // clang-format on

    private:
    ///\cond HIDDEN_SYMBOLS
    void load_actual_value();

    QTimer *timer = nullptr;

    QList<FieldEntry> field_entries;
    Data_engine *data_engine = nullptr;
    QGridLayout *grid_layout = nullptr;

    QString empty_value_placeholder{"/"};

    bool is_enabled = true;
    bool is_visible = true;

    void start_timer();

    void resizeMe(QResizeEvent *event) override;

    QMetaObject::Connection callback_timer = {};
    void set_ui_visibility();

    void set_labels_enabled();
    void set_total_visibilty();
    const int BLINK_INTERVAL_MS = 500;
    bool init_ok = false;
    ///\endcond
};
/** \} */ // end of group ui
#endif    // DATAENGINEINPUT_H
