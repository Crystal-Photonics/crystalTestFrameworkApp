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
    Image(UI_container *parent, QString script_path);
    ~Image();
    ///\endcond

    void load_image_file(const std::string &path_to_image);


    void set_visible(bool visible);
private:
	Aspect_ratio_label *label = nullptr;
    QImage image;
    void load_image(const std::string &path_to_image_);
    QString script_path;
};

#endif // IMAGE_H
