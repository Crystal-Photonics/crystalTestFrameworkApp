#ifndef PLOT_H
#define PLOT_H

#include <QObject>
#include <functional>
#include <memory>
#include <vector>

class Event_filter;
class Plot;
class QAction;
class QColor;
class QPointF;
class QSplitter;
class QwtPickerClickPointMachine;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotPicker;

class Curve {
    public:
    ///\cond HIDDEN_SYMBOLS
    Curve(QSplitter *, Plot *plot);
    Curve(Curve &&other) = delete;
    ~Curve();
    void add(const std::vector<double> &data);
    ///\endcond

    void append_point(double x, double y); //!< \brief Appends a point to a curve
                                           //!< \param x               Double value of the x position.
                                           //!< \param y               Double value of the y position.
                                           //!< \sa add_spectrum()
                                           //!< \sa add_spectrum_at()
                                           //!< \details \par examples:
                                           //!< \code
                                           //! local plot = Ui.Plot.new()
                                           //! local curve = plot:add_curve()
                                           //! curve:append_point(1,1)
                                           //! curve:append_point(2,1) -- plots a line
                                           //! \endcode

    void add_spectrum_at(const unsigned int spectrum_start_channel,
                         const std::vector<double> &data); //!< \brief Adds spectrum data to a curve at a specific position.
                                                           //!< \param spectrum_start_channel Integer value. Position where to place the spectrum data.
                                                           //!< \param data                Table of doubles which will be plotted.
                                                           //!< \sa add_spectrum()
                                                           //!< \sa add_point()
                                                           //!< \details If the new data overlaps with already existing data of the curve the
                                                           //! overlapping values will be mathematically added. Not defined segments of the curve
                                                           //! are assumed to be zero.
                                                           //! \par examples:
                                                           //!< \code
                                                           //! local plot = Ui.Plot.new()
                                                           //! local curve = plot:add_curve()
                                                           //! curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
                                                           //! curve:add_spectrum_at(12,{101,102,103,104,105,106,107,108,109,110,101})
                                                           //!  -- plots another sawtooth
                                                           //!  -- plot data is now:
                                                           //!  -- {1,2,3,4,5,6,7,8,9,10,1,101,102,103,104,105,106,107,108,109,110,101}
                                                           //! \endcode
                                                           //! In this example the curve will plot the values:
                                                           //! \code
                                                           //! {1,2,3,4,5,6,7,8,9,10,1,101,102,103,104,105,106,107,108,109,110,101}
                                                           //! \endcode
                                                           //! This function can be used to update just a segment of a spectrum.

    void add_spectrum(const std::vector<double> &data); //!< \brief Adds spectrum data ta a curve
                                                        //!< \param data                Table of doubles which will be plotted.
                                                        //!< \sa add_spectrum_at()
                                                        //!< \sa add_point()
                                                        //!< \details If the new data overlaps with already existing data of the curve the
                                                        //! overlapping values will be mathematically added. Not defined segments of the curve
                                                        //! are assumed to be zero.
                                                        //!  \par examples:
                                                        //!< \code
                                                        //! local plot = Ui.Plot.new()
                                                        //! local curve = plot:add_curve()
                                                        //! curve:add_spectrum( { 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 1}) -- plots a sawtooth
                                                        //! curve:add_spectrum( {10,12,13,14,15,16,17,18,19,20,11}) -- update sawtooth
                                                        //! --plot data is now: {11,14,16,18,20,22,24,26,28,30,12}
                                                        //! \endcode
                                                        //!

    void clear(); //!< \brief Clears the data of the curve.
                  //!< \details \par examples:
                  //!< \code
                  //! local plot = Ui.Plot.new()
                  //! local curve = plot:add_curve()
                  //! curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
                  //! curve:clear() -- no line is plotted since all data is cleared
                  //! curve:add_spectrum( {1,2,3,4,5,6,7,8,9,10,1}) -- sawtooth appears again
                  //! --plot data is now: {1,2,3,4,5,6,7,8,9,10,1}
                  //! \endcode
                  //!

    void set_x_axis_offset(double offset); //!< \brief Sets the offset of the plot x axis.
                                           //!< \param offset               double value. Will be the x axis offset.
                                           //!< \sa set_gain()
                                           //!< \details \par examples:
                                           //!< \code
                                           //! local plot = Ui.Plot.new()
                                           //! local curve = plot:add_curve()
                                           //! curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
                                           //! curve:set_offset(20)
                                           //! -- draws the curve with the value 1 at x-position 20, 2 at 21, 3 at 22 and so forth.
                                           //! \endcode
                                           //! This function only affects the drawing of the curve. Internally the points would be
                                           //! still at the position 1,2,3,4 and so on.

    void set_x_axis_gain(double gain); //!< \brief Sets the scale of the plot x axis.
                                       //!< \param gain               double value. Will be the x axis offset.
                                       //!< \sa set_offset()
                                       //!< \details \par examples:
                                       //!< \code
                                       //! local plot = Ui.Plot.new()
                                       //! local curve = plot:add_curve()
                                       //! curve:add_spectrum({1,2,3,4,5,6,7,8,9,10,1}) -- plots a sawtooth
                                       //! curve:set_offset(7)
                                       //! curve:set_gain(2)
                                       //! -- draws the curve with the value 1 at x-position 9, 2 at 11, 3 at 13 and so forth.
                                       //! \endcode
                                       //! This function only affects the drawing of the curve. Internally the points would be
                                       //! still at the position 1,2,3,4 and so on.

    double
    integrate_ci(double integral_start_ci,
                 double integral_end_ci); //!< \brief Returns the integral of the curve between the array indexes \c integral_start_ci and \c integral_end_ci
                                          //!< \param integral_start_ci     Positive integer. Start-index of the points.
                                          //!< \param integral_end_ci       Positive integer. End-index of the points.
                                          //!< \details \par examples:
                                          //!< \code
                                          //! local plot = Ui.Plot.new()
                                          //! local curve = plot:add_curve()
                                          //! curve:add_point(5,20)
                                          //! curve:add_point(10,5)
                                          //! curve:add_point(15,3)
                                          //! curve:add_point(20,50)
                                          //! local result = curve:integrate_ci(2,3)    --sums the values 5 and 3 = 8
                                          //! \endcode
                                          //! TODO: Stimmt das mit dem Index?
                                          //!

    void set_median_enable(bool enable); //!< \brief Enables the built in median smoothing filter of the plot drawing function
                                         //!< \param enable       Whether enabled(true) or not(false).
                                         //!< \details As default the kernel size is 3. The filter runs a boxcar function with the length of \c
                                         //! kernel_size over the curve, sorts its values, takes the value from the middle and
                                         //! use this value as a point of the curve. A boxcar of eg. {5,9,3} results in 5. The smoothing
                                         //! just happens in the drawing function. If you would save the curve data, the saved data will
                                         //! be the raw data of the curve.
                                         //!< \sa set_median_kernel_size()
                                         //!< \par examples:
                                         //!< \code
                                         //! local plot = Ui.Plot.new()
                                         //! local curve = plot:add_curve()
                                         //! curve:set_enable_median(true)       -- enable median
                                         //! curve:set_median_kernel_size(5)      -- median kernel size = 5
                                         //! \endcode
                                         //!

    void set_median_kernel_size(unsigned int kernel_size); //!<\brief Sets the kernel size of the median filter
                                                           //!< \param kernel_size       positive integer => 3. Value must be odd.
                                                           //! The effect of \c kernel_size is described more detailed in set_median_enable().
                                                           //!< \sa set_median_enable()
                                                           //!< \par examples:
                                                           //!< \code
                                                           //! local plot = Ui.Plot.new()
                                                           //! local curve = plot:add_curve()
                                                           //! curve:set_enable_median(true)       -- enable median
                                                           //! curve:set_median_kernel_size(5)      -- median kernel size = 5
                                                           //! \endcode

    void set_color_by_name(const std::string &name); //!<\brief Sets the color of the curve by name.
                                                     //!< \param name Name as string of the color.
                                                     //! Names are described in:<br>
    //!< <a href="https://www.w3.org/TR/SVG/types.html#ColorKeywords">https://www.w3.org/TR/SVG/types.html#ColorKeywords</a><br>
    //!< \sa set_color_by_rgb()
    //!< \par examples:
    //! "aqua" "blue" "green" "red" "yellow" "white" "black" <br>
    //!< \code
    //! local plot = Ui.Plot.new()
    //! local curve = plot:add_curve()
    //! curve:set_color_by_name("red")
    //! \endcode

    void set_color_by_rgb(int r, int g, int b); //!<\brief Sets the color of the curve by rgb value.
                                                //!< \param r Red integer value(0-255).
                                                //!< \param g Green integer value(0-255).
                                                //!< \param b Blue integer value(0-255).
                                                //!< \sa set_color_by_name()
                                                //!< \par examples:
                                                //!< \code
                                                //! local plot = Ui.Plot.new()
                                                //! local curve = plot:add_curve()
                                                //! curve:set_color_by_name(255,0,0)     -- red
                                                //! curve:set_color_by_name(255,255,0)   -- orange
                                                //! curve:set_color_by_name(255,255,255) -- white
                                                //! \endcode
												///\cond HIDDEN_SYMBOLS
    void set_color(const QColor &color);
	void set_onetime_click_callback(std::function<void(double, double)> click_callback);
    ///\endcond

    private:
    ///\cond HIDDEN_SYMBOLS
    void resize(std::size_t size);
    void update();
    void detach();

    Plot *plot{nullptr};
    QwtPlotCurve *curve{nullptr};
    std::vector<double> xvalues{};
    std::vector<double> yvalues_orig{};
    std::vector<double> yvalues_plot{};
    bool median_enable{false};
    unsigned int median_kernel_size{3};
    double offset{0};
    double gain{1};
	Event_filter *event_filter{nullptr};
	///\endcond
    friend class Plot;
};

class Plot {
    public:
    ///\cond HIDDEN_SYMBOLS
    Plot(QSplitter *parent);
    Plot(Plot &&other);
    Plot &operator=(Plot &&other);
    ~Plot();
///\endcond
//! \brief Generates a new curve attached to the plot.
//! \return The generated curve.
//! \sa Curve
//! \par examples:
//! \code
//! local plot = Ui.Plot.new()
//! local curve = plot:add_curve()
//! curve:append_point(1,1)
//! curve:append_point(2,1) -- plots a line
//! \endcode
#ifdef DOXYGEN_ONLY
    //this block is just for ducumentation purpose
    curve add_curve();
#endif

    void clear(); //!<\brief Deletes all curves from a plot.
                  //!< \sa Curve
                  //!< \par examples:
                  //!< \code
                  //! local plot = Ui.Plot.new()
                  //! local curve = plot:add_curve()
                  //! plot.clear(); -- deletes all curves. Plot is empty now
                  //! \endcode
                  //!TODO: Testen was passiert wenn man auf Kurve zugreift nachdem man clear aufgerufen hat.

    private:
    void update();
    void set_rightclick_action();

	QwtPlot *plot{nullptr};
	QAction *save_as_csv_action{nullptr};
	QwtPlotPicker *picker{nullptr};
	QwtPickerClickPointMachine *clicker{nullptr};
	std::vector<Curve *> curves{};
	int curve_id_counter{0};

    friend class Curve;
};

#endif // PLOT_H
