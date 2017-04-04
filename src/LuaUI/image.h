#ifndef IMAGE_H
#define IMAGE_H

#include <QImage>

class UI_container;
class QLabel;
class QWidget;

class Image {
    public:
    ///\cond HIDDEN_SYMBOLS
	Image(UI_container *parent);
    ~Image();
    ///\endcond

    void load_image_file(const std::string path_to_image);


    private:
    QLabel *label = nullptr;
    QWidget *base_widget = nullptr;
    QImage image;
    void load_image(const std::string path_to_image);
};

#endif // IMAGE_H
