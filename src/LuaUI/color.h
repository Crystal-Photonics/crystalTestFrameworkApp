#ifndef COLOR_H
#define COLOR_H

#include <string>

struct Color {
    static Color Color_from_name(const std::string &name);  //!<\brief Creates a color by name.
                                                            //!< \param name Name as string of the color.
                                                            //! Names are described in:<br>
                                                            //! <a href="https://www.w3.org/TR/SVG/types.html#ColorKeywords">https://www.w3.org/TR/SVG/types.html#ColorKeywords</a><br>
                                                            //!< \sa Curve::set_color()
                                                            //!< \sa Plot::set_x_marker()
                                                            //!< \par examples:
                                                            //! "aqua" "blue" "green" "red" "yellow" "white" "black" <br>
                                                            //!< \code
                                                            //! local plot = Ui.Plot.new()
                                                            //! local curve = plot:add_curve()
                                                            //! local color = Ui.Color_from_name("red")
                                                            //! curve:set_color(color)
                                                            //! \endcode

    static Color Color_from_r_g_b(int r, int g, int b);     //!<\brief Creates a color by rgb values.
                                                            //!< \param r Red integer value(0-255).
                                                            //!< \param g Green integer value(0-255).
                                                            //!< \param b Blue integer value(0-255).
                                                            //!< \sa set_color_by_name()
                                                            //!< \sa Curve::set_color()
                                                            //!< \sa Plot::set_x_marker()
                                                            //!< \par examples:
                                                            //!< \code
                                                            //! local plot = Ui.Plot.new()
                                                            //! local curve = plot:add_curve()
                                                            //! local color = Ui.Color_from_r_g_b(255,0,0)  --red
                                                            //! color = Ui.Color_from_r_g_b(255,255,0)      --orange
                                                            //! color = Ui.Color_from_r_g_b(255,255,255)    --white
                                                            //! curve:set_color(color)
                                                            //! \endcode

    static Color Color_from_rgb(int rgb);                   //!<\brief Creates a color by rgb values.
                                                            //!< \param rgb Integer value of the usual rgb color bitmask.
                                                            //!< \sa set_color_by_name()
                                                            //!< \sa Curve::set_color()
                                                            //!< \sa Plot::set_x_marker()
                                                            //!< \par examples:
                                                            //!< \code
                                                            //! local plot = Ui.Plot.new()
                                                            //! local curve = plot:add_curve()
                                                            //! local color = Ui.Color_from_rgb(0xFF0000)  --red
                                                            //! color = Ui.Color_from_rgb(0xFFFF00)      --orange
                                                            //! color = Ui.Color_from_rgb(0xFFFFFF)    --white
                                                            //! curve:set_color(color)
                                                            //! \endcode
    ///\cond HIDDEN_SYMBOLS
    int rgb{};
    ///\endcond
};

#endif // COLOR_H
