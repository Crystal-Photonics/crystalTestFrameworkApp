#ifndef IMAGE_H
#define IMAGE_H

#include "ui_container.h"
#include <QImage>

class QLabel;
class QWidget;
class Aspect_ratio_label;
/** \ingroup ui
\{
    \class   Image

    \brief Puts an image in the script-ui area. You can use it as a part of instructions when the user needs to do something.
  \image html Image.png Image
  \image latex Image.png Image
  \image rtf Image.png Image
 */
class Image : public UI_widget {
    public:
#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    Image();
#endif
    /// \cond HIDDEN_SYMBOLS
    Image(UI_container *parent, QString script_path);
    ~Image();
    /// \endcond
    // clang-format off
  /*! \fn  Image();
      \brief Creates an image object.
      \par examples:
      \code
          local ui_image = Ui.Image.new();
          ui_image:load_image_file("Jellyfish.jpg")
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    load_image_file(string path_to_image);
#endif
    /// \cond HIDDEN_SYMBOLS
    void load_image_file(const std::string &path_to_image);
    /// \endcond
    // clang-format off
  /*! \fn  load_image_file(string path_to_image);
      \brief loads the image from file. If a relative path is used, the current directory is the directory of the script path
      \param path_to_image the path the image. If a relative path is used, the current directory is the directory of the script path.
      \par examples:
      \code
          local ui_image = Ui.Image.new();
          ui_image:load_image_file("Jellyfish.jpg")
      \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_maximum_height(number height_pixels);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_maximum_height(int height);
    /// \endcond
    // clang-format off
  /*! \fn  set_maximum_height(number height_pixels);
	  \brief Sets the limit of the size of the image by its height. Overwrites previous limit. The default limit is twice the size of the image.
	  \param height_pixels the desired maximum height of the image in pixels
	  \par examples:
	  \code
		  local ui_image = Ui.Image.new();
		  ui_image:set_maximum_height(300)
	  \endcode
  */
    // clang-format on

#ifdef DOXYGEN_ONLY
    // this block is just for ducumentation purpose
    set_maximum_width(number width_pixels);
#endif
    /// \cond HIDDEN_SYMBOLS
    void set_maximum_width(int width);
    /// \endcond
    // clang-format off
  /*! \fn  set_maximum_width(number width_pixels);
	  \brief Sets the limit of the size of the image by its width. Overwrites previous limit. The default limit is twice the size of the image.
	  \param width_pixels the desired maximum width of the image in pixels
	  \par examples:
	  \code
		  local ui_image = Ui.Image.new();
		  ui_image:set_maximum_width(300)
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
      \brief sets the visibility of the image object.
      \param visible the state of the visibility. (true / false)
          \li If \c false: the image is hidden
          \li If \c true: the image is visible (default)
      \par examples:
      \code
          local ui_image = Ui.Image.new();
          ui_image:load_image_file("Jellyfish.jpg")
          ui_image:set_visible(false)   -- image is hidden
          ui_image:set_visible(true)   -- image is visible
      \endcode
  */

    // clang-format on
    private:
    ///\cond HIDDEN_SYMBOLS
    Aspect_ratio_label *label = nullptr;
    QImage image;
    void load_image(const std::string &path_to_image_);
    QString script_path;
    ///\endcond
};
/** \} */ // end of group ui
#endif    // IMAGE_H
