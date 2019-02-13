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
///\cond HIDDEN_SYMBOLS

class Aspect_ratio_label : public QLabel {
    public:
    Aspect_ratio_label(QWidget *widget)
        : QLabel(widget) {
        assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    }
    bool hasHeightForWidth() const override {
        return aspect_ratio != 0.;
    }
    int heightForWidth(int width) const override {
        return static_cast<int>(std::round(width * aspect_ratio));
    }
    void setPixmap(const QPixmap &pixmap) {
        QLabel::setPixmap(pixmap);
        aspect_ratio = static_cast<double>(pixmap.height()) / pixmap.width();
        assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    }

    private:
    double aspect_ratio{};
};

Image::Image(UI_container *parent, QString script_path)
    : UI_widget{parent}
    , label{new Aspect_ratio_label(parent)} {
    this->script_path = script_path;
    parent->add(label, this);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    label->setScaledContents(true);
    parent->scroll_to_bottom();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

Image::~Image() {}

void Image::load_image_file(const std::string &path_to_image) {
    load_image(path_to_image);
}

void Image::set_visible(bool visible) {
    label->setVisible(visible);
}

void Image::load_image(const std::string &path_to_image_) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    auto path_to_image = get_absolute_file_path(script_path, path_to_image_);
    if (QFile::exists(QString::fromStdString(path_to_image))) {
        image.load(QString::fromStdString(path_to_image));
        label->setPixmap(QPixmap::fromImage(image));
    } else {
        QString msg = QString{"cant open Image file \"%1\""}.arg(QString::fromStdString(path_to_image));
        throw sol::error(msg.toStdString());
    }
}
