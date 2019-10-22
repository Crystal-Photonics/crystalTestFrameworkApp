#ifndef PLOT_H
#define PLOT_H

#include "color.h"

#include "scriptengine.h"
#include "ui_container.h"
#include <QObject>
#include <functional>
#include <memory>
#include <vector>

namespace Utility {
    class Event_filter;
}
class Plot;
class QAction;
class QColor;
class QPointF;
class QSplitter;
class QwtPickerClickPointMachine;
class QwtPickerTrackerMachine;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotPicker;
struct Curve_data;
class UI_container;

/** \ingroup ui
\{
    \class  Curve
    \brief  A lineplot-curve used inside a Plot. You can not create a curve directly. You can create it only via Plot::add_curve()
    \sa Plot
*/

class Curve {
    public:
    ///\cond HIDDEN_SYMBOLS
    Curve(UI_container *, ScriptEngine *script_engine, Plot *plot);
    Curve(Curve &&other) = delete;
    ~Curve();
    void add(const std::vector<double> &data);
    ///\endcond

    ///\cond HIDDEN_SYMBOLS
    void append_point(double x, double y);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    append_point(double x, double y);
#endif
    // clang-format off
  /*! \fn  append_point(double x, double y);
    \brief Appends a point to a curve
    \param x Double value of the x  position.
    \param y Double value of the y position.
    \sa add_spectrum()
    \sa add_spectrum_at()
    \details \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:append_point(1,1)
        curve:append_point(2,1) -- plots a line
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void add_spectrum_at(const unsigned int spectrum_start_channel, const std::vector<double> &data);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    add_spectrum_at(int spectrum_start_channel, number_table data);
#endif
    // clang-format off
  /*! \fn add_spectrum_at(int spectrum_start_channel,  number_table data);
        \brief Adds \glos{spectrum} data to a curve at a specific position.
        \param spectrum_start_channel Integer value > 0. Position where to place the \glos{spectrum} data. The first element has an index of 1.
        \param data Table of doubles which will be plotted.
        \sa add_spectrum()
        \sa append_point()
        \details If the new data overlaps with already existing
        data of the curve the
        overlapping values will be mathematically added. Not
        defined segments of the curve are assumed to be zero.
        \par examples:
        \code
            local plot = Ui.Plot.new()
            local curve = plot:add_curve()
            curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
            curve:add_spectrum_at(1,{101,102,103,104,105,106,107,108,109,110,101})
            -- plots another sawtooth
            -- plot data is now:
            -- {1,2,3,4,5,6,7,8,9,10,102,102,103,104,105,106,107,108,109,110,101}
        \endcode
        In this example the curve will plot the values:
        \code
            {1,2,3,4,5,6,7,8,9,10,102,102,103,104,105,106,107,108,109,110,101}
        \endcode
        This function can be used to update just a segment of a \glos{spectrum}.
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void add_spectrum(const std::vector<double> &data);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    add_spectrum(number_table data);
#endif
    // clang-format off
  /*! \fn add_spectrum(number_table data);
    \brief Adds \glos{spectrum} data ta a curve
    \param data Table of doubles which will be plotted.
    \sa add_spectrum_at()
    \sa append_point()
    \details If the new data overlaps with already existing data of the curve the
    overlapping values will be mathematically added. Not
    defined segments of the curve are assumed to be zero.
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:add_spectrum( { 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 1}) -- plots a sawtooth
        curve:add_spectrum( {10,12,13,14,15,16,17,18,19,20,11}) -- update sawtooth
        --plot data is now: {11,14,16,18,20,22,24,26,28,30,12}
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void clear();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    clear();
#endif
    // clang-format off
  /*! \fn clear();
    \brief Clears the data of the curve.
    \details
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
        curve:clear() -- no line is plotted since all data is cleared
        curve:add_spectrum( {1,2,3,4,5,6,7,8,9,10,1}) -- sawtooth appears again
        --plot data is now: {1,2,3,4,5,6,7,8,9,10,1}
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_x_axis_offset(double offset);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_x_axis_offset(double offset);
#endif
    // clang-format off
  /*! \fn set_x_axis_offset(double offset);
    \brief Sets the offset of the plot x axis.
    \param offset double value. Will be the x  axis offset.
    \sa set_gain()
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
        curve:set_offset(20)
        -- draws the curve with the value 1 at x-position 20, 2 at 21, 3 at 22 and so forth.
    \endcode
    This function only affects the drawing of the curve. Internally the points
    would be still at the position 1,2,3,4 and so on.
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_x_axis_gain(double gain);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_x_axis_gain(double gain);
#endif
    // clang-format off
  /*! \fn set_x_axis_gain(double gain);
        \brief Sets the scale of the plot x axis.
        \param gain double value. Will be the x axis offset.
        \sa set_offset()
        \par examples:
        \code
            local plot = Ui.Plot.new()
            local curve = plot:add_curve()
            curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
            curve:set_offset(7)
            curve:set_gain(2)
            -- draws the curve with the x-value
                --1 at 9(1*2+7),
                --2 at 11(2*2+7),
                --3 at 13(3*2+7) and so forth.
        \endcode
        This function only affects the drawing of the curve. Internally the points
        would be still at the position 1,2,3,4 and so on.
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    double integrate_ci(double integral_start_ci, double integral_end_ci);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    double integrate_ci(double integral_start_ci, double integral_end_ci);
#endif
    // clang-format off
  /*! \fn double integrate_ci(double integral_start_ci, double integral_end_ci);
        \brief Returns the integral of the curve between the array indexes \c integral_start_ci and \c integral_end_ci
        \param integral_start_ci Positive integer. Start-index of the points. The first element has the index 1.
        \param integral_end_ci Positive integer. End-index of the points. The first element has the index 1.
        \par examples:
        \code
            local plot = Ui.Plot.new()
            local curve = plot:add_curve()
            curve:append_point(5,20)
            curve:append_point(10,5)
            curve:append_point(15,3)
            curve:append_point(20,50)
            local result = curve:integrate_ci(2,3)    --sums the values 5 and 3 = 8
            print(result)
        \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_median_enable(bool enable);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_median_enable(bool enable);
#endif
    // clang-format off
  /*! \fn set_median_enable(bool enable);
    \brief Enables the built in median smoothing filter of the plot drawing function
    \param enable Whether enabled(true) or not(false).
    \details As default the kernel size is 3. The filter runs a boxcar function with
    the length of \c kernel_size over the curve, sorts its values, takes the value
    from the middle and use this value as a point of the curve. A boxcar of eg.
    {5,9,3} results in 5. The smoothing just happens in the drawing function. If you
    would save the curve data, the saved data will be the raw data of the curve.
    \sa set_median_kernel_size()
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:set_enable_median(true)       -- enable median
        curve:set_median_kernel_size(5)      -- median kernel size = 5
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_median_kernel_size(unsigned int kernel_size);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_median_kernel_size(unsigned int kernel_size);
#endif
    // clang-format off
  /*! \fn set_median_kernel_size(unsigned int kernel_size);
    \brief Sets the kernel size of the median filter
    \param kernel_size       positive integer => 3. Value must be odd.

    The effect of \c kernel_size is described more
    detailed in Curve::set_median_enable().
    \sa set_median_enable()
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:set_enable_median(true)       -- enable median
        curve:set_median_kernel_size(5)      -- median kernel size = 5
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_color(const Color &color);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_color(Color color);
#endif
    // clang-format off
  /*! \fn set_color(Color color);
    \brief Sets the color of the curve.
    \param color Color.
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        local color = Ui.Color("red")
        curve:set_color(color) --turns curve red
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    double pick_x_coord();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    double user_pick_x_coord();
#endif
    // clang-format off
  /*! \fn double user_pick_x_coord();
    \brief waits until the user has clicked on the plot. Then it returns the x value of the
    point the user has clicked at. This function can be used to let the user identify e.g.
    a peak inside a \glos{spectrum}.
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
        local result = curve:user_pick_x_coord() --waits until user clicks at the plot
        print("x-value: "..result)
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    sol::table get_y_values_as_array();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    number_table get_y_values_as_array();
#endif
    // clang-format off
  /*! \fn number_table get_y_values_as_array();
    \brief returns the y-values of the curve as a table.
    \returns the y-values of the curve as a table.
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
        local result = curve:get_y_values_as_array()
        print(result) --prints {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1}
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_onetime_click_callback(std::function<void(double, double)> click_callback);
    ///\endcond
    private:
    ///\cond HIDDEN_SYMBOLS

    void update();
    void detach();
    Curve_data &curve_data();

    ///\endcond
    Plot *plot{nullptr};
    QwtPlotCurve *curve{nullptr};
    Utility::Event_filter *event_filter{nullptr};
    ///\endcond
    friend class Plot;
    ScriptEngine *script_engine_;
};
/** \} */ // end of group ui

/** \ingroup ui
    \class  Plot
    \brief  An interface to a plot object.
    \sa Curve
*/

class Plot : public UI_widget {
    public:
    ///\cond HIDDEN_SYMBOLS
    Plot(UI_container *parent);
    Plot(Plot &&other) = delete;
    Plot &operator=(Plot &&other);
    ~Plot();
    ///\endcond

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    Plot();
#endif
    // clang-format off
  /*! \fn  Plot();
    \brief Generates a new curve attached to the plot.
    \return The generated curve.
    \sa Curve
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:append_point(1,1)
        curve:append_point(2,1) -- plots a line
    \endcode
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void clear();
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    clear();
#endif
    // clang-format off
  /*! \fn clear();
    \brief Deletes all curves from a plot.
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        plot.clear(); -- deletes all curves. Plot is empty now
    \endcode
    TODO: Testen was passiert wenn man auf Kurve zugreift
    nachdem man clear aufgerufen hat.
  */
    // clang-format on

    ///\cond HIDDEN_SYMBOLS
    void set_x_marker(const std::string &title, double xpos, const Color &color);
    ///\endcond
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_x_marker(string &title, double xpos, Color color);
#endif
    // clang-format off
  /*! \fn set_x_marker(string &title, double xpos, Color color);
        \brief Adds a vertical marker to the plot to mark a x-position.
        \param title A string value.
        \param xpos A number.
        \param color Color with the type Color.
        \par examples:
        \code
            local plot = Ui.Plot.new()
            plot:set_x_marker(10,
            Ui.Color("blue"))
        \endcode
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
      \brief sets the visibility of the plot object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the plot object is hidden
          \li If \c true: the plot object is visible (default)
      \par examples:
      \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        curve:append_point(1,1)
        curve:append_point(2,1) -- plots a line

        plot:set_visible(false)   -- plot object is hidden
        plot:set_visible(true)   -- plot object is visible
      \endcode
  */
    // clang-format on

    private:
    /// \cond HIDDEN_SYMBOLS
    void update();
    void set_rightclick_action();

    QwtPlot *plot{nullptr};
    QAction *save_as_csv_action{nullptr};
    QwtPlotPicker *picker{nullptr};
    QwtPlotPicker *track_picker{nullptr};
    QwtPickerClickPointMachine *clicker{nullptr};
    QwtPickerTrackerMachine *tracker{nullptr};
    std::vector<Curve *> curves{};
    int curve_id_counter{0};

    friend class Curve;
    /// \endcond
};
/** \} */ // end of group ui
#endif    // PLOT_H
