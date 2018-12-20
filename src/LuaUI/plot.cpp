#include "plot.h"
#include "config.h"
#include "qt_util.h"
#include "ui_container.h"
#include "util.h"


#include "Windows/mainwindow.h"
#include "scriptengine.h"
#include <QAction>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>
#include <QSplitter>
#include <QtGlobal>
#include <fstream>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

#include <chrono>

using namespace std::chrono;

///\cond HIDDEN_SYMBOLS

struct Curve_data : QwtSeriesData<QPointF> {
    size_t size() const override;
    QPointF sample(size_t i) const override;
    QRectF boundingRect() const override;

    void append(double x, double y);
    void resize(std::size_t size);
    void add(const std::vector<double> &data);
    void add_spectrum(int spectrum_start_channel, const std::vector<double> &data);
    void clear();
    void set_gain(double gain);
    void set_offset(double offset);
    void update();
    const std::vector<double> &get_plot_data();

    bool median_enable{false};
    unsigned int median_kernel_size{3};

    private:
    void update_bounding_rect_height();
    void update_bounding_rect_height(double new_yvalue);
    bool use_interpolated_values() const;

    std::vector<double> xvalues{};
    std::vector<double> yvalues_orig{};
    std::vector<double> yvalues_plot{};
    QRectF bounding_rect{};
    double offset{0};
    double gain{1};
};

size_t Curve_data::size() const {
    return xvalues.size();
}

QPointF Curve_data::sample(size_t i) const {
    return {xvalues[i], use_interpolated_values() ? yvalues_plot[i] : yvalues_orig[i]};
}

QRectF Curve_data::boundingRect() const {
    return bounding_rect;
}

void Curve_data::append(double x, double y) {
    if (xvalues.empty()) {
        bounding_rect.setX(x);
        bounding_rect.setY(y);
        bounding_rect.setWidth(0);
        bounding_rect.setHeight(0);
    }
    bounding_rect.setRight(x);
    //bounding_rect.setHeight(std::max(y - bounding_rect.y(), bounding_rect.height()));
    xvalues.push_back(x);
    yvalues_orig.push_back(y);
    update_bounding_rect_height(y);
}

void Curve_data::resize(std::size_t size) {
    if (xvalues.size() > size) {
        xvalues.resize(size);
        yvalues_orig.resize(size);
        yvalues_plot.resize(size);
    } else if (xvalues.size() < size) {
        auto old_size = xvalues.size();
        xvalues.resize(size);
        for (auto i = old_size; i < size; i++) {
            xvalues[i] = offset + gain * i;
        }
        yvalues_orig.resize(size, 0.);
        yvalues_plot.resize(size, 0.);
        bounding_rect.setRight(xvalues.back());
    }
}

void Curve_data::add(const std::vector<double> &data) {
    resize(data.size());
    std::transform(std::begin(data), std::end(data), std::begin(yvalues_orig), std::begin(yvalues_orig), std::plus<>());
    update_bounding_rect_height();
}

void Curve_data::add_spectrum(int spectrum_start_channel, const std::vector<double> &data) {
    size_t s = std::max(xvalues.size(), data.size() + spectrum_start_channel);
    resize(s);
    std::transform(std::begin(data), std::end(data), std::begin(yvalues_orig) + spectrum_start_channel, std::begin(yvalues_orig) + spectrum_start_channel,
                   std::plus<>());
    update_bounding_rect_height();
}

void Curve_data::clear() {
    xvalues.clear();
    yvalues_orig.clear();
    bounding_rect.adjust(0, 0, 0, 0);
}

void Curve_data::set_gain(double gain) {
    this->gain = gain;
    xvalues.clear();
    resize(yvalues_orig.size());
    resize(yvalues_plot.size());
}

void Curve_data::set_offset(double offset) {
    this->offset = offset;
    xvalues.clear();
    resize(yvalues_orig.size());
}

void Curve_data::update() {
    if (use_interpolated_values()) {
        std::vector<double> kernel(median_kernel_size);
        const unsigned int HALF_KERNEL_SIZE = median_kernel_size / 2;

        for (unsigned int i = 0; i < HALF_KERNEL_SIZE; i++) {
            yvalues_plot[i] = yvalues_orig[i];
        }

        for (unsigned int i = yvalues_orig.size() - HALF_KERNEL_SIZE; i < yvalues_orig.size(); i++) {
            yvalues_plot[i] = yvalues_orig[i];
        }

        for (unsigned int i = HALF_KERNEL_SIZE; i < yvalues_orig.size() - HALF_KERNEL_SIZE; i++) {
            for (unsigned int j = 0; j < median_kernel_size; j++) {
                kernel[j] = yvalues_orig[i + j - HALF_KERNEL_SIZE];
            }
            std::sort(kernel.begin(), kernel.end());
            yvalues_plot[i] = kernel[HALF_KERNEL_SIZE];
        }
    }
}

void Curve_data::update_bounding_rect_height() {
    qDebug() << "old top:" << bounding_rect.top() << "old bot: " << bounding_rect.bottom();
    auto max_iter = std::max_element(std::begin(yvalues_orig), std::end(yvalues_orig));
    double max_value = *max_iter;
    auto min_iter = std::min_element(std::begin(yvalues_orig), std::end(yvalues_orig));
    double min_value = *min_iter;
    bounding_rect.setBottom(max_value); //top and bottom are swapped in QRect for some reason
    bounding_rect.setTop(min_value);
    qDebug() << "top:" << bounding_rect.top() << "bot: " << bounding_rect.bottom();
}

void Curve_data::update_bounding_rect_height(double new_yvalue) {
    qDebug() << "old nv top:" << bounding_rect.top() << "old nv bot: " << bounding_rect.bottom();
    if (bounding_rect.top() > new_yvalue) { //top and bottom are swapped in QRect for some reason
        bounding_rect.setTop(new_yvalue);
    } else if (bounding_rect.bottom() < new_yvalue) {
        bounding_rect.setBottom(new_yvalue);
    }
    qDebug() << "nv top:" << bounding_rect.top() << "nv bot: " << bounding_rect.bottom();
}

const std::vector<double> &Curve_data::get_plot_data() {
    return yvalues_plot;
}

bool Curve_data::use_interpolated_values() const {
    return median_enable && (median_kernel_size < yvalues_orig.size());
}

Curve::Curve(UI_container *, ScriptEngine *script_engine, Plot *plot)
    : plot(plot)
    , curve(new QwtPlotCurve)
    , event_filter(new Utility::Event_filter(plot->plot->canvas()))
    , script_engine_{script_engine} {
    curve->attach(plot->plot);
    curve->setTitle("curve" + QString::number(plot->curve_id_counter++));
    plot->curves.push_back(this);
    plot->plot->canvas()->installEventFilter(event_filter);
    curve->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, false);
    curve->setData(new Curve_data);
}

Curve::~Curve() {
    if (plot) {
        auto &curves = plot->curves;
        curves.erase(std::find(std::begin(curves), std::end(curves), this));
        detach();
    }
}
void Curve::add(const std::vector<double> &data) {
    curve_data().add(data);
    update();
}
///\endcond
void Curve::append_point(double x, double y) {
    curve_data().append(x, y);
    update();
}

void Curve::add_spectrum_at(const unsigned int spectrum_start_channel, const std::vector<double> &data) {
    curve_data().add_spectrum(spectrum_start_channel, data);
    update();
}

void Curve::clear() {
    curve_data().clear();
    update();
}

void Curve::set_x_axis_offset(double offset) {
    curve_data().set_offset(offset);
    update();
}

void Curve::set_x_axis_gain(double gain) {
    curve_data().set_gain(gain);
    update();
}

double Curve::integrate_ci(double integral_start_ci, double integral_end_ci) {
#if 1
    double result = 0;
    integral_start_ci -= 1; //coming from lua convention: "startindex of array is 1"
    integral_end_ci -= 1;
    if (integral_start_ci < 0) {
        integral_start_ci = 0;
    }

    if (integral_end_ci < 0) {
        integral_end_ci = 0;
    }
    unsigned int s = round(integral_start_ci);
    unsigned int e = round(integral_end_ci);

    const auto &yvalues_plot = curve_data().get_plot_data();

    if (s >= yvalues_plot.size()) {
        s = yvalues_plot.size() - 1;
    }

    if (e >= yvalues_plot.size()) {
        e = yvalues_plot.size() - 1;
    }

    for (unsigned int i = s; i <= e; i++) {
        result += yvalues_plot[i];
    }
    return result;
#endif
    return 0;
}

sol::table Curve::get_y_values_as_array() {
#if 1

    assert(MainWindow::gui_thread != QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    const auto &yvalues_plot = curve_data().get_plot_data();

    auto retval = script_engine_->create_table();
    for (auto val : yvalues_plot) {
        retval.add(val);
    }
    return retval;

#endif
    return 0;
}

void Curve::set_median_enable(bool enable) {
    curve_data().median_enable = enable;
    update();
}

void Curve::set_median_kernel_size(unsigned int kernel_size) {
    if (kernel_size & 1) {
        curve_data().median_kernel_size = kernel_size;
        update();
    } else {
        //TODO: Tell the user that the call had no effect?
    }
}

void Curve::set_color(const Color &color) {
    curve->setPen(QColor(color.rgb));
}

double Curve::pick_x_coord() {
//  QMetaObject::Connection callback_connection =  QObject::connect(plot, &QLineEdit::returnPressed, [this] { this->script_engine->ui_event_queue_send(); });
// script_engine->ui_event_queue_run();
// QObject::disconnect(callback_connection);
#if 0
    event_filter->add_callback([ this ](QEvent * event) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouse_event = static_cast<QMouseEvent *>(event);
            const auto &pixel_pos = mouse_event->pos();
            auto x_pos = plot->plot->invTransform(QwtPlot::xBottom, pixel_pos.x());
            auto y_pos = plot->plot->invTransform(QwtPlot::yLeft, pixel_pos.y());
            this->script_engine->ui_event_queue_send();
            event_filter->clear();
            return true;
        }
        return false;
    });
#endif
    script_engine_->ui_event_queue_run();
    return 0;
}

///\cond HIDDEN_SYMBOLS
void Curve::set_onetime_click_callback(std::function<void(double, double)> click_callback) {
    event_filter->add_callback([ callback = std::move(click_callback), this ](QEvent * event) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouse_event = static_cast<QMouseEvent *>(event);
            const auto &pixel_pos = mouse_event->pos();
            auto x_pos = plot->plot->invTransform(QwtPlot::xBottom, pixel_pos.x());
            auto y_pos = plot->plot->invTransform(QwtPlot::yLeft, pixel_pos.y());
            callback(x_pos, y_pos);
            event_filter->clear();
            return true;
        }
        return false;
    });
}

void Curve::update() {
    curve_data().update();
    plot->update();
}

void Curve::detach() {
    event_filter->clear();
}

Curve_data &Curve::curve_data() {
    return static_cast<Curve_data &>(*curve->data());
}

Plot::Plot(UI_container *parent)
    : UI_widget{parent}
    , plot(new QwtPlot)
    , picker(new QwtPlotPicker(plot->canvas()))
    , track_picker(new QwtPlotPicker(plot->canvas()))
    , clicker(new QwtPickerClickPointMachine)
    , tracker(new QwtPickerTrackerMachine) {
    clicker->setState(clicker->PointSelection);
    parent->add(plot, this);
    plot->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    plot->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    set_rightclick_action();
    picker->setStateMachine(clicker);
    picker->setTrackerMode(QwtPicker::ActiveOnly);
    track_picker->setStateMachine(tracker);
    track_picker->setTrackerMode(QwtPicker::AlwaysOn);
    parent->scroll_to_bottom();
}

Plot::~Plot() {
    for (auto &curve : curves) {
        curve->detach();
        curve->plot = nullptr;
    }
    //the plot was using xvalues and yvalues directly, but now they are gone
    //this is to make the plot own the data
}
///\endcond
void Plot::clear() {
    curves.clear();
}

void Plot::set_x_marker(const std::string &title, double xpos, const Color &color) {
    //
    auto marker = new QwtPlotMarker{title.c_str()};
    if (title.empty() == false) {
        marker->setLabel(QString::fromStdString(title));
    }
    marker->setXValue(xpos);
    marker->setLinePen(QColor(color.rgb));
    marker->setLineStyle(QwtPlotMarker::LineStyle::VLine);
    marker->setLabelOrientation(Qt::Orientation::Vertical);
    marker->attach(plot);
#if 0
    const int Y_AXIS_STEP = 10;
    int i = 0;
    double plot_y_min = plot->axisScaleDiv(QwtPlot::Axis::yLeft).lowerBound();
    double plot_y_max = plot->axisScaleDiv(QwtPlot::Axis::yLeft).upperBound();
    double plot_y_range = plot_y_max - plot_y_min;
    //plot_y_range += plot_y_range/Y_AXIS_STEP;
    const QwtPlotItemList &itmList = plot->itemList();
    //qDebug() << "ymarker calc:";
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        QwtPlotItem *item = *it;
        if (item->isVisible() && item->rtti() == QwtPlotItem::Rtti_PlotMarker) {
            QwtPlotMarker *m = dynamic_cast<QwtPlotMarker *>(item);
            if (m) {
                i++;
                double y_value = plot_y_min + (i % Y_AXIS_STEP) * plot_y_range / Y_AXIS_STEP;
                //qDebug() << "ymarker: " << y_value;
                m->setYValue(y_value);
            }
        }
    }
#endif
    update();
}

void Plot::set_visible(bool visible) {
    plot->setVisible(visible);
}

void Plot::update() {
    plot->replot();
}

void Plot::set_rightclick_action() {
    delete save_as_csv_action;
    save_as_csv_action = new QAction(plot);
    save_as_csv_action->setText(QObject::tr("save_as_csv"));
    if (curves.size()) {
        std::vector<QwtPlotCurve *> raw_curves;
        raw_curves.resize(curves.size());
        std::transform(std::begin(curves), std::end(curves), std::begin(raw_curves), [](const Curve *curve) { return curve->curve; });
        QObject::connect(save_as_csv_action, &QAction::triggered, [ plot = this->plot, curves = std::move(raw_curves) ] {
            QString last_dir = QSettings{}.value(Globals::last_csv_saved_directory_key, QDir::currentPath()).toString();
            auto dir = QFileDialog::getExistingDirectory(plot, QObject::tr("Select folder to save data in"), last_dir);
            if (dir.isEmpty() == false) {
                QSettings{}.setValue(Globals::last_csv_saved_directory_key, dir);
                for (auto &curve : curves) {
                    auto filename = QDir(dir).filePath(curve->title().text() + ".csv");
                    std::ofstream f{filename.toStdString()};
                    auto data = curve->data();
                    auto size = data->size();
                    for (std::size_t i = 0; i < size; i++) {
                        const auto &point = data->sample(i);
                        f << point.x() << ';' << point.y() << '\n';
                    }
                    f.flush();
                    if (!f) {
                        QMessageBox::critical(plot, QObject::tr("Error"), QObject::tr("Failed writing to file %1").arg(filename));
                    }
                }
            }

        });
    } else {
        save_as_csv_action->setEnabled(false);
    }
    plot->addAction(save_as_csv_action);
}
