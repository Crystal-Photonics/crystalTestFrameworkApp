#ifndef ISOTOPESOURCESELECTOR_H
#define ISOTOPESOURCESELECTOR_H

#include "ui_container.h"
#include <QDateTime>
#include <QList>
#include <QMetaObject>
#include <functional>
#include <string>

class QComboBox;

/// \cond HIDDEN_SYMBOLS
class IsotopeSource {
    public:
    QString serial_number;
    QString isotope;
    QString normal_user;
    QDate start_date;
    double start_activity_becquerel;

    double half_time_days;
    double get_activtiy_becquerel(QDate date_for_activity);
};
/// \endcond
/** \ingroup ui
\{
    \class   IsotopeSourceSelector
    \brief An IsotopeSourceSelector can be used to keep track of the used radioactive sources and
    show the radioactive sources which are stored in the database to the user. This database contains the
    serial number, isotope element, the creation date and the activity at the creation date. After
    a certain isotope is selected you can use the IsotopeSourceSelector object to calculate its
    current activity based on its half life.
  \image html IsotopeSourceSelector.png IsotopeSourceSelector with all defined isotopes as values
  \image latex IsotopeSourceSelector.png IsotopeSourceSelector with all defined isotopes as values
  \image rtf IsotopeSourceSelector.png IsotopeSourceSelector with all defined isotopes as values
    \sa \ref radioactive_source_database
 */
class IsotopeSourceSelector : public UI_widget {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    IsotopeSourceSelector();
#endif
    /// \cond HIDDEN_SYMBOLS
    IsotopeSourceSelector(UI_container *parent);
    ~IsotopeSourceSelector();
    /// \endcond
    // clang-format off
  /*! \fn  IsotopeSourceSelector()
      \brief Creates an IsotopeSourceSelector object.
      \par examples:
      \code
            local source_selector = Ui.IsotopeSourceSelector.new()
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    double get_selected_activity_Bq();
#endif
    /// \cond HIDDEN_SYMBOLS
    double get_selected_activity_Bq();
    /// \endcond
    // clang-format off
  /*! \fn  get_selected_activity_Bq()
      \brief Returns the current activity in Becquerel of the selected radioactive source. It uses:

         \f[
        current\_activity\_becquerel = \left( start\_activity\_becquerel \times e^ {-age\_days \frac{\ln (2)}{halftime\_days}} \right)
          \f]
      \par
with
        \li \f$ halftime\_days_c_o_5_7 = 271 + \frac{17}{24}\f$ (271 days and 17 hours)
        \li \f$ halftime\_days_n_a_2_2 = 2.6 \times 365\f$  (2.6 years)
        \li \f$ halftime\_days_i_1_2_9 = 1.57e7 \times 365 \f$ (1.57 * 10^7  years)

    \returns the current activity in Becquerel of the selected radioactive source.

      \par examples:
      \code
            local source_selector = Ui.IsotopeSourceSelector.new()
            source_selector:filter_by_isotope("co57") --lists only "co57" isotope sources
            local done_button = Ui.Button.new("Next")
            done_button:await_click()
            print("You selected: " ..source_selector:get_selected_serial_number())
            print("activity[kBq]: " .. round(source_selector:get_selected_activity_Bq()/1000,2))
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_selected_serial_number();
#endif
    /// \cond HIDDEN_SYMBOLS
    std::string get_selected_serial_number();
    /// \endcond
    // clang-format off
  /*! \fn  string get_selected_serial_number()
      \brief Returns the serial number of the selected radioactive source.

    \returns the serial number of the selected radioactive source.

      \par examples:
      \code
            local source_selector = Ui.IsotopeSourceSelector.new()
            local done_button = Ui.Button.new("Next")
            done_button:await_click()
            print("You selected the radioactive source: " ..source_selector:get_selected_serial_number()) -- e.g. AH1226
            print("activity[kBq]: " .. round(source_selector:get_selected_activity_Bq()/1000,2))
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    string get_selected_name();
#endif
    /// \cond HIDDEN_SYMBOLS
    std::string get_selected_name();
    /// \endcond
    // clang-format off
  /*! \fn  string get_selected_name();
      \brief Returns the name of the selected radioactive source.

    \returns the name of the selected radioactive source.

      \par examples:
      \code
            local source_selector = Ui.IsotopeSourceSelector.new()
            local done_button = Ui.Button.new("Next")
            done_button:await_click()
            print("You selected the isotope: " ..source_selector:get_selected_name()) -- "co57"
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    filter_by_isotope(string isotope_name);
#endif
    /// \cond HIDDEN_SYMBOLS
    void filter_by_isotope(std::string isotope_name);
    /// \endcond
    // clang-format off
  /*! \fn  filter_by_isotope();
      \brief Lists only radioactive sources of a certain isotope.

    \param isotope_name the name of the isotope which should be filtered for.

    Current isotopes:
    \li "co57"
    \li "na22"
    \li "i129"

      \par examples:
      \code
            local source_selector = Ui.IsotopeSourceSelector.new()
            source_selector:filter_by_isotope("co57") --lists only "co57" isotope sources
            local done_button = Ui.Button.new("Next")
            done_button:await_click()
            print("You selected the radioactive source: " ..source_selector:get_selected_serial_number()) -- e.g. AH1226
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
      \brief sets the visibility of the IsotopeSourceSelector object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the IsotopeSourceSelector object is hidden
          \li If \c true: the IsotopeSourceSelector object is visible (default)
      \par examples:
      \code
          local source_selector = Ui.IsotopeSourceSelector.new()
          source_selector:set_visible(false)   -- IsotopeSourceSelector object is hidden
          source_selector:set_visible(true)   -- IsotopeSourceSelector object is visible
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
      \brief sets the enable state of the IsotopeSourceSelector object.
      \param enable whether enabled or not. (true / false)
          \li If \c false: the IsotopeSourceSelector object is disabled and gray
          \li If \c true: the IsotopeSourceSelector object enabled (default)
      \par examples:
      \code
          local source_selector = Ui.IsotopeSourceSelector.new()
          source_selector:set_enabled(false)   --  IsotopeSourceSelector object is disabled
          source_selector:set_enabled(true)   --  IsotopeSourceSelector object is enabled
      \endcode
  */
    // clang-format on

    /// \cond HIDDEN_SYMBOLS
    private:
    void load_isotope_database();
    void load_most_recent();
    void save_most_recent();
    void connect_isotope_selecteted();
    void disconnect_isotope_selecteted();

    QComboBox *combobox = nullptr;
    QMetaObject::Connection callback_isotope_selected = {};
    void set_single_shot_return_pressed_callback(std::function<void()> callback);

    void fill_combobox_with_isotopes(QString isotope_name);
    QList<IsotopeSource> isotope_sources;
    IsotopeSource get_source_by_serial_number(QString serial_number);
    QString isotope_filter_m;
    /// \endcond
};
/** \} */ // end of group ui
#endif    // ISOTOPESOURCESELECTOR_H
