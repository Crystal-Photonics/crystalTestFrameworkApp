#ifndef IMAGE_H
#define IMAGE_H

#include <QLabel>
#include <QSplitter>
#include <QImage>

class Image {
    public:
    ///\cond HIDDEN_SYMBOLS
    Image(QSplitter *parent);
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
