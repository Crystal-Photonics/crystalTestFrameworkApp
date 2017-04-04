#include "image.h"
#include "ui_container.h"

#include <QFile>
#include <QImage>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <sol.hpp>

///\cond HIDDEN_SYMBOLS
Image::Image(UI_container *parent) {
    base_widget = new QWidget(parent);
    label = new QLabel(base_widget);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(label);
    base_widget->setLayout(layout);
	parent->add_below(base_widget);

    base_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    label->setScaledContents(true);
}

Image::~Image() {}

void Image::load_image_file(const std::string path_to_image) {
    load_image(path_to_image);
}

void Image::load_image(const std::string path_to_image) {
    if (QFile::exists(QString::fromStdString(path_to_image))) {
        image.load(QString::fromStdString(path_to_image));
        label->setPixmap(QPixmap::fromImage(image));
    } else {
        QString msg = QString{"cant open Image file \"%1\""}.arg(QString::fromStdString(path_to_image));
        throw sol::error(msg.toStdString());
    }
}
