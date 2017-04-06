#ifndef IMAGE_H
#define IMAGE_H

#include <QImage>

class UI_container;
class QLabel;
class QWidget;
class Aspect_ratio_label;

class Image {
    public:
    ///\cond HIDDEN_SYMBOLS
	Image(UI_container *parent);
    ~Image();
    ///\endcond

    void load_image_file(const std::string path_to_image);


    private:
	Aspect_ratio_label *label = nullptr;
    QWidget *base_widget = nullptr;
    QImage image;
    void load_image(const std::string path_to_image);
};

#endif // IMAGE_H
