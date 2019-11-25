///\cond HIDDEN_SYMBOLS
#include "image.h"
#include "scriptengine.h"
#include "ui_container.h"

#include "Windows/mainwindow.h"
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <sol.hpp>

class Aspect_ratio_label : public QLabel {
	static constexpr auto max_scaling_factor = 2; //how much a picture can be scaled up.
    public:
    Aspect_ratio_label(QWidget *widget)
        : QLabel(widget) {
		assert(MainWindow::gui_thread == QThread::currentThread());
    }
    bool hasHeightForWidth() const override {
		return pixmap();
    }
    int heightForWidth(int width) const override {
		if (not pixmap()) {
			return 0;
		}
		const auto size = pixmap()->size();
		return width * size.height() / size.width();
    }
	QSize sizeHint() const override {
		if (pixmap()) {
			return pixmap()->size();
		}
		return {0, 0};
	}
	QSize minimumSizeHint() const override {
		if (not pixmap()) {
			return {0, 0};
		}
		const auto min_width = std::min(100, pixmap()->size().width());
		return {heightForWidth(min_width), min_width};
	}
    void setPixmap(const QPixmap &pixmap) {
		assert(MainWindow::gui_thread == QThread::currentThread());
        QLabel::setPixmap(pixmap);
		setMinimumSize({0, 0});
		setMaximumSize(pixmap.size() * max_scaling_factor);
    }
};

Image::Image(UI_container *parent, QString script_path)
    : UI_widget{parent}
    , label{new Aspect_ratio_label(parent)} {
	assert(MainWindow::gui_thread == QThread::currentThread());
    this->script_path = script_path;
	label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	label->setScaledContents(true);
	parent->add(label, this);
}

Image::~Image() {}

void Image::load_image_file(const std::string &path_to_image) {
	load_image(path_to_image);
}

void Image::set_maximum_height(int height) {
	if (auto pixmap = label->pixmap()) {
		const auto size = pixmap->size();
		const auto width = height * size.width() / size.height();
		label->setMaximumSize(width, height);
	} else {
		qDebug() << "Warning: Setting maximum size for image before loading an image has no effect";
	}
}

void Image::set_maximum_width(int width) {
	if (auto pixmap = label->pixmap()) {
		label->setMaximumSize(width, label->heightForWidth(width));
	} else {
		qDebug() << "Warning: Setting maximum size for image before loading an image has no effect";
	}
}

void Image::set_visible(bool visible) {
    label->setVisible(visible);
}

void Image::load_image(const std::string &path_to_image_) {
	assert(MainWindow::gui_thread == QThread::currentThread());
    auto path_to_image = get_absolute_file_path(script_path, path_to_image_);
    if (QFile::exists(QString::fromStdString(path_to_image))) {
        image.load(QString::fromStdString(path_to_image));
        label->setPixmap(QPixmap::fromImage(image));
    } else {
		QString msg = QString{"can't open Image file \"%1\""}.arg(QString::fromStdString(path_to_image));
        throw sol::error(msg.toStdString());
    }
}
/// \endcond
