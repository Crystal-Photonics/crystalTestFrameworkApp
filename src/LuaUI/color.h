#ifndef COLOR_H
#define COLOR_H

#include <string>

/** \ingroup ui
 *  \{
 */

// clang-format off
/*!
  \class   Color
  \author Tobias Rieger (tr@crystal-photonics.com),<br> Arne Krüger
  (ak@crystal-photonics.com)
  \brief Type holding a color value
  \par
    This type is used mainly for defining the color of a plot curve.
  */
// clang-format on

struct Color {

#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  Color(string name);
#endif
  /// @cond HIDDEN_SYMBOLS
  Color(const std::string &name);
  /// @endcond
  // clang-format off
/*! \fn Color(string name)
    \brief Creates a color by name.
    \param name Name as string of the color.
    Names are described in:<br>
    <a href="https://www.w3.org/wiki/CSS/Properties/color/keywords">www.w3.org/wiki/CSS/Properties/color/keywords</a><br>
     \sa Curve::set_color()
     \sa Plot::set_x_marker()
     \par examples:
        "aqua" "blue" "green" "red" "yellow" "white" "black" <br>
     \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        local color = Ui.Color("red")
        curve:set_color(color)
    \endcode
*/

  // clang-format on
#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  Color(int r, int g, int b);
#endif
  /// \cond HIDDEN_SYMBOLS
  Color(int r, int g, int b);
  /// \endcond
  // clang-format off
/*! \fn Color(int r, int g, int b)
    \brief Creates a color by rgb values.
    \param r Red integer value(0-255).
    \param g Green integer value(0-255).
    \param b Blue integer value(0-255).
        \sa set_color_by_name()
        \sa Curve::set_color()
        \sa Plot::set_x_marker()
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        local color = Ui.Color(255,0,0)  --red
        color = Ui.Color(255,255,0)      --orange
        color = Ui.Color(255,255,255)    --white
        curve:set_color(color)
    \endcode
*/
  // clang-format on
#ifdef DOXYGEN_ONLY
  // this block is just for ducumentation purpose
  Color(int rgb);
#endif
  /// \cond HIDDEN_SYMBOLS
  Color(int rgb);
  /// \endcond
  // clang-format off
/*! \fn Color(int rgb)
    \brief Creates a color by rgb values.
    \param rgb Integer value of the usual rgb color bitmask.
    \sa set_color_by_name()
    \sa Curve::set_color()
    \sa Plot::set_x_marker()
    \par examples:
    \code
        local plot = Ui.Plot.new()
        local curve = plot:add_curve()
        local color = Ui.Color(0xFF0000)  --red
        color = Ui.Color(0xFFFF00)      --orange
        color = Ui.Color(0xFFFFFF)    --white
        curve:set_color(color)
    \endcode
*/
  ///\cond HIDDEN_SYMBOLS
  int rgb{};
  ///\endcond
};
/** \} */ // end of group ui
#endif    // COLOR_H
