///\cond HIDDEN_SYMBOLS
#include "plot.h"
#include "CommunicationDevices/communicationdevice.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "testrunner.h"
#include "ui_container.h"
#include "util.h"

#include <QAction>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QtGlobal>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <qwt_date_scale_draw.h>
#include <qwt_date_scale_engine.h>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_draw.h>

using namespace std::chrono;

struct Curve_data : QwtSeriesData<QPointF> {
    size_t size() const override;
    QPointF sample(std::size_t i) const override;
    QRectF boundingRect() const override;

    std::pair<double, double> point_at(std::size_t i) const;
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

    friend class Curve;
};

size_t Curve_data::size() const {
    return xvalues.size();
}

QPointF Curve_data::sample(size_t i) const {
    assert(i < xvalues.size());
    assert(i < yvalues_plot.size());
    assert(i < yvalues_orig.size());
    return {xvalues[i], use_interpolated_values() ? yvalues_plot[i] : yvalues_orig[i]};
}

QRectF Curve_data::boundingRect() const {
    return bounding_rect;
}

std::pair<double, double> Curve_data::point_at(std::size_t i) const {
    assert(i < xvalues.size());
    assert(i < yvalues_plot.size());
    assert(i < yvalues_orig.size());
    return {xvalues[i], yvalues_orig[i]};
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
    } else if (yvalues_plot.size() < size) {
        yvalues_plot.resize(size, 0.);
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
        if (yvalues_plot.size() == 0) {
            throw std::runtime_error(QObject::tr("Internal plot error: yvalues_plot.size() == 0").toStdString());
        }

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
    } else {
        yvalues_plot = yvalues_orig;
    }
}

void Curve_data::update_bounding_rect_height() {
    auto max_iter = std::max_element(std::begin(yvalues_orig), std::end(yvalues_orig));
    double max_value = *max_iter;
    auto min_iter = std::min_element(std::begin(yvalues_orig), std::end(yvalues_orig));
    double min_value = *min_iter;
    bounding_rect.setBottom(max_value); //top and bottom are swapped in QRect for some reason
    bounding_rect.setTop(min_value);
}

void Curve_data::update_bounding_rect_height(double new_yvalue) {
    if (bounding_rect.top() > new_yvalue) { //top and bottom are swapped in QRect for some reason
        bounding_rect.setTop(new_yvalue);
    } else if (bounding_rect.bottom() < new_yvalue) {
        bounding_rect.setBottom(new_yvalue);
    }
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
    , script_engine{script_engine} {
    assert(currently_in_gui_thread());
    curve->attach(plot->plot);
    curve->setTitle("curve" + QString::number(plot->curve_id_counter++));
    plot->curves.push_back(this);
    plot->plot->canvas()->installEventFilter(event_filter);
    plot->curve_added(this);
    curve->setRenderHint(QwtPlotItem::RenderHint::RenderAntialiased, false);
    curve->setData(new Curve_data);
}

Curve::~Curve() {
    if (plot) {
        auto &curves = plot->curves;
        auto pos = std::find(std::begin(curves), std::end(curves), this);
        assert(pos != std::end(curves)); //if this assert fires, someone has removed a curve from plot->curves who forgot to detach first
        curves.erase(pos);
        detach();
    }
}

void Curve::add(const std::vector<double> &data) {
    add_spectrum_at(0, data);
}

void Curve::append_point(double x, double y) {
    if (not plot) {
        throw std::runtime_error{"Curve is not associated with a plot"};
    }
    curve_data().append(x, y);
    plot->value_added(x, y);
    update();
}

void Curve::append(double y) {
    if (not plot) {
        throw std::runtime_error{"Curve is not associated with a plot"};
    }
    if (plot->using_time_scale) {
        append_point(QDateTime::currentMSecsSinceEpoch(), y);
    } else {
        if (curve_data().size()) {
            append_point(curve_data().xvalues.back() + 1, y);
        } else {
            append_point(0, y);
        }
    }
}

void Curve::add_spectrum_at(const unsigned int spectrum_start_channel, const std::vector<double> &data) {
    if (data.empty()) {
        return;
    }
    if (not plot) {
        throw std::runtime_error{"Curve is not associated with a plot"};
    }
    curve_data().add_spectrum(spectrum_start_channel, data);
    const auto &curve_y_values = curve_data().use_interpolated_values() ? curve_data().yvalues_plot : curve_data().yvalues_orig;
    //border x values
    plot->value_added(curve_data().xvalues.front() + spectrum_start_channel, curve_y_values.back());
    plot->value_added(curve_data().xvalues.front() + spectrum_start_channel + data.size(), curve_y_values.back());
    //border y values
    const auto [min_it, max_it] = std::minmax_element(std::begin(curve_y_values) + spectrum_start_channel, std::end(curve_y_values));
    const auto minindex = min_it - std::begin(curve_y_values);
    const auto maxindex = max_it - std::begin(curve_y_values);
    plot->value_added(curve_data().xvalues[minindex], *min_it);
    plot->value_added(curve_data().xvalues[maxindex], *max_it);
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
        result += yvalues_plot.at(i);
    }
    return result;
#else
    return 0;
#endif
}

sol::table Curve::get_y_values_as_array() {
    const auto &yvalues_plot = curve_data().get_plot_data();

    auto retval = script_engine->create_table();
    for (auto val : yvalues_plot) {
        retval.add(val);
    }
    return retval;
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

void Curve::set_line_width(double pixel) {
    auto pen = curve->pen();
    pen.setWidth(pixel);
    curve->setPen(pen);
}

double Curve::pick_x_coord() {
    assert(not currently_in_gui_thread());
    if (not plot) {
        throw std::runtime_error{"Curve is not associated with a plot"};
    }
    QMetaObject::Connection callback_connection;
    double xcoord;
    bool clicked = false;
    auto connector = Utility::RAII_do(
        [&callback_connection, this, &xcoord, &clicked] {
            callback_connection = QObject::connect(plot, &Plot::point_clicked, [this, &xcoord, &clicked](QPointF point) {
                clicked = true;
                xcoord = point.x();
                script_engine->post_ui_event();
            });
        },
        [&callback_connection] { QObject::disconnect(callback_connection); });
    while (true) {
        const auto event = script_engine->await_ui_event();
        if (clicked) {
            return xcoord;
        }
        if (event == Event_id::interrupted) {
            throw std::runtime_error{"interrupted"};
        }
    }
}

void Curve::update() {
    assert(currently_in_gui_thread());
    if (not plot) {
        throw std::runtime_error{"Curve is not associated with a plot"};
    }
    curve_data().update();
    plot->update();
}

void Curve::detach() {
    event_filter->clear();
}

Curve_data &Curve::curve_data() {
    return static_cast<Curve_data &>(*curve->data());
}

class TimePicker : public QwtPlotPicker {
    public:
    TimePicker(QWidget *parent)
        : QwtPlotPicker{parent} {}
    QwtText trackerTextF(const QPointF &point) const override {
        if (scale_engine) {
            return scale_engine->toDateTime(point.x()).toString() + "\n" + QString::number(point.y());
        }
        return QString::number(point.x()) + "\n" + QString::number(point.y());
    }
    QwtDateScaleEngine *scale_engine = nullptr;
};

static QRectF operator*(const QRectF &rect, double value) {
    const auto &new_width = rect.width() * value;
    const auto width_change = new_width - rect.width();
    const auto &new_height = rect.height() * value;
    const auto height_change = new_height - rect.height();
    return QRectF{rect.left() - width_change / 2, rect.top() - height_change / 2, new_width, new_height};
}

static QRectF operator/(const QRectF &rect, double value) {
    return rect * (1 / value);
}

static QRectF &operator*=(QRectF &rect, double value) {
    return rect = rect * value;
}

static QRectF &operator/=(QRectF &rect, double value) {
    return rect = rect / value;
}

struct Zoomer_controller : QObject {
    Zoomer_controller(QwtPlotZoomer *zoomer, Plot *parent_plot)
        : QObject{zoomer->plot()}
        , parent_plot{parent_plot}
        , zoomer{zoomer}
        , export_button{parent_plot->export_button} {}
    bool eventFilter(QObject *watched, QEvent *event) override final {
        const auto zoom_factor = 1.25;
        const auto move_factor = .2; //move in [move_factor * size] steps
        switch (event->type()) {
            case QEvent::KeyRelease: {
                const auto key_event = static_cast<QKeyEvent *>(event);
                auto rect = zoomer->zoomRect();
                switch (key_event->key()) {
                    case Qt::Key::Key_PageUp:
                    case Qt::Key::Key_Plus:
                        rect /= zoom_factor;
                        auto_scrolling = false;
                        break;
                    case Qt::Key::Key_PageDown:
                    case Qt::Key::Key_Minus:
                        rect *= zoom_factor;
                        auto_scrolling = false;
                        break;
                    case Qt::Key::Key_Right:
                        rect.moveLeft(rect.left() + rect.width() * move_factor);
                        auto_scrolling = false;
                        break;
                    case Qt::Key::Key_Left:
                        rect.moveLeft(rect.left() - rect.width() * move_factor);
                        auto_scrolling = false;
                        break;
                    case Qt::Key::Key_Up:
                        rect.moveTop(rect.top() + rect.height() * move_factor);
                        auto_scrolling = false;
                        break;
                    case Qt::Key::Key_Down:
                        rect.moveTop(rect.top() - rect.height() * move_factor);
                        auto_scrolling = false;
                        break;
                    case Qt::Key::Key_Home:
                    case Qt::Key::Key_End:
                        if (not first) {
                            rect.setLeft(xmin - 1);
                            rect.setRight(xmax + 1);
                            rect.setBottom(ymin - 1);
                            rect.setTop(ymax + 1);
                        }
                        auto_scrolling = true;
                        break;
                    default:
                        return QObject::eventFilter(watched, event);
                }
                zoomer->zoom(rect);
            }
                return true;
            case QEvent::Wheel: {
                const auto wheel_event = static_cast<QWheelEvent *>(event);
                if (wheel_event->buttons() == Qt::MouseButton::LeftButton || wheel_event->modifiers() != Qt::KeyboardModifier::NoModifier) {
                    return true;
                }
                auto_scrolling = false;
                auto rect = zoomer->zoomRect();
                // magic number 1.00186 comes from "120th root of x = 1.25" which comes from the need to make one scroll wheel move equal to pressing + or - to make
                // zooming steps with mouse wheel equivalent to zooming with keys
                rect /= std::pow(1.0018612595916019726587729883803, wheel_event->angleDelta().y());
                zoomer->zoom(rect);
                return true;
            }
            case QEvent::MouseButtonPress: {
                const auto mouse_event = static_cast<QMouseEvent *>(event);
                if (mouse_event->buttons() == Qt::MouseButton::LeftButton) {
                    auto_scrolling = false;
                    if (mouse_event->modifiers() == Qt::KeyboardModifier::NoModifier) {
                        x_scaling = plot()->canvasMap(zoomer->xAxis());
                        y_scaling = plot()->canvasMap(zoomer->yAxis());
                        const auto mouse_drag_start_coordinate = mouse_event->globalPos();
                        plot_drag_start_coordinate = mouse_coords_to_plot_coords(mouse_drag_start_coordinate);
                        plot_start_coordinate = zoomer->zoomRect().topLeft();
                        if (parent_plot) {
                            parent_plot->point_clicked(plot_drag_start_coordinate);
                        }
                        return true;
                    }
                }
                if (mouse_event->buttons() == Qt::MouseButton::MiddleButton) {
                    zoom_to_home();
                    return true;
                }
            } break;
            case QEvent::MouseButtonRelease: {
                const auto mouse_event = static_cast<QMouseEvent *>(event);
                if (mouse_event->button() == Qt::MouseButton::RightButton && not curves.empty()) {
                    QMenu menu;
                    QAction action_run(tr("Save as .csv"));
                    connect(&action_run, &QAction::triggered, [this] {
                        const auto last_dir = QDir{QSettings{}.value(Globals::last_csv_saved_directory_key, QDir::currentPath()).toString()};
                        qDebug() << "Last dir:" << QSettings{}.value(Globals::last_csv_saved_directory_key, QDir::currentPath()).toString();
                        const auto filename = QFileDialog::getSaveFileName(MainWindow::mw, tr("Select file to save plot data in"),
                                                                           last_dir.filePath(curves.front()->title().text() + ".csv"), "CSV (*.csv)");
                        if (filename.isEmpty()) {
                            return;
                        }
                        auto new_dir = QDir{filename};
                        new_dir.cdUp();
                        qDebug() << "New dir:" << new_dir.path();
                        QSettings{}.setValue(Globals::last_csv_saved_directory_key, new_dir.path());
                        std::ofstream f{filename.toStdString()};
                        std::size_t elements = 0;

                        //formatting constants
                        const char separator = ',';
                        const auto max_precision = 17;

                        for (auto &curve : curves) {
                            elements = std::max(elements, curve->data()->size());
                            f << curve->title().text().toStdString() << separator << Color(curve->pen().color().rgb()).name() << separator;
                        }
                        f << '\n' << std::fixed << std::setprecision(max_precision);
                        for (std::size_t i = 0; i < elements; i++) {
                            for (auto &curve : curves) {
                                const auto data = dynamic_cast<Curve_data *>(curve->data());
                                assert(data);
                                const auto size = data->size();
                                if (i < size) {
                                    const auto [x, y] = data->point_at(i);
                                    if (using_time_scale) {
                                        f << std::setprecision(3) << x / 1000. << std::setprecision(max_precision) << separator << y << separator;
                                    } else {
                                        f << x << separator << y << separator;
                                    }
                                } else {
                                    f << separator << separator;
                                }
                            }
                            f << '\n';
                        }
                        f.flush();
                        if (not f) {
                            QMessageBox::critical(plot(), QObject::tr("Error"), QObject::tr("Failed writing to file %1").arg(filename));
                        }
                    });
                    menu.addAction(&action_run);
                    menu.exec(mouse_event->globalPos());
                    return true;
                }
                if (mouse_event->button() == Qt::MouseButton::MiddleButton) {
                    //Doesn't do anything, but is important to return true, otherwise the event is handled by other event handlers and we scroll wrong
                    return true;
                }
            } break;
            case QEvent::MouseMove: {
                const auto mouse_event = static_cast<QMouseEvent *>(event);
                if (mouse_event->buttons() == Qt::MouseButton::LeftButton && mouse_event->modifiers() == Qt::KeyboardModifier::NoModifier) {
                    const auto mouse_plot_coords = mouse_coords_to_plot_coords(mouse_event->globalPos());
                    auto zoom_rect = zoomer->zoomRect();
                    zoom_rect.moveTo(plot_start_coordinate + plot_drag_start_coordinate - mouse_plot_coords);
                    zoomer->zoom(zoom_rect);
                }
                break;
            }
            case QEvent::Resize: {
                const auto resize_event = static_cast<QResizeEvent *>(event);
                export_button->move(resize_event->size().width(), 0);
            } break;
            default:
                break;
        }
        return QObject::eventFilter(watched, event);
    }
    void value_added(double x, double y) {
        if (first) {
            xmin = xmax = x;
            ymin = ymax = y;
            first = false;
            zoom_to_home();
            return;
        }
        xmin = std::min(x, xmin);
        xmax = std::max(x, xmax);
        ymin = std::min(y, ymin);
        ymax = std::max(y, ymax);
        if (auto_scrolling) {
            zoom_to_home();
        }
    }
    void zoom_to_home() {
        QKeyEvent event{QEvent::KeyRelease, Qt::Key::Key_End, Qt::KeyboardModifier::NoModifier};
        eventFilter(nullptr, &event);
    }

    std::vector<QwtPlotCurve *> curves;
    Plot *parent_plot; //Note: This becomes nullptr after the script stops, do not rely on this pointer to be valid.
    bool using_time_scale = false;

    private:
    QwtPlot *plot() const {
        return zoomer->plot();
    }

    QPointF mouse_coords_to_plot_coords(const QPoint &mouse_coords) {
        const auto widget_pos = plot()->canvas()->mapFromGlobal(mouse_coords);
        return QwtScaleMap::invTransform(x_scaling, y_scaling, widget_pos);
    }

    QwtPlotZoomer *zoomer;
    double xmin, ymin, xmax, ymax;
    QwtScaleMap x_scaling, y_scaling;
    QPointF plot_drag_start_coordinate;
    QPointF plot_start_coordinate;
    QPushButton *export_button;
    bool first = true;
    bool auto_scrolling = true;
};

struct Plot_export_data {
    std::string plot_text;
    std::string test_name;
    std::string device_string;
    QPushButton *export_button;
    Plot *plot;
};

void export_plot(QwtPlot *plot, Plot_export_data &plot_data, QPushButton *export_button) {
    static const auto font = QFont("Arial", 10);
#ifdef WIN32
    constexpr static const char *root_prefix = "C:";
#else
    constexpr static const char *root_prefix = "";
#endif
    const auto proposed_filename = QSettings{}.value(Globals::recent_plot_export_path, root_prefix).toString() +
                                   QString{"/plot-%1.png"}.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss"));
    const auto file = QFileDialog::getSaveFileName(plot, QString::fromStdString("CrystalTestFramework - Select file for export"), proposed_filename,
                                                   QObject::tr("Images (*.png *.jpg)"));
    if (file.isEmpty()) {
        return;
    }
    QSettings{}.setValue(Globals::recent_plot_export_path, QFileInfo(file).path());
    QPixmap pm{plot->size()};
    export_button->hide();
    plot->render(&pm);
    export_button->show();
    QPainter p{&pm};
    p.setFont(font);
    p.drawText(5, QFontMetrics{font}.height(), QString::fromStdString(plot_data.plot_text));
    pm.save(file);
    const auto text_filename = QString{file}.replace(QRegularExpression{R"(\.[^\.]{3,4}$)"}, ".txt");
    std::ofstream f{text_filename.toStdString()};
    f << "Test: " << plot_data.test_name << '\n';
    f << "Devices: " << (plot_data.plot ? plot_data.plot->scriptengine->device_list_string() : plot_data.device_string) << '\n';
}

Plot::Plot(UI_container *parent, ScriptEngine *scriptengine)
    : UI_widget{parent}
    , plot(new QwtPlot)
    , picker(new QwtPlotPicker{plot->canvas()})
    , track_picker(new TimePicker{plot->canvas()})
    , clicker(new QwtPickerClickPointMachine)
    , tracker(new QwtPickerTrackerMachine)
    , export_button{new QPushButton(plot)}
    , scriptengine{scriptengine} {
    export_button->setIcon(QIcon::fromTheme("document-save", QIcon{"://src/icons/icons8-save-48.png"}));
    export_button->raise();
    auto plot_data_up = std::make_unique<Plot_export_data>();
    plot_data = plot_data_up.get();
    plot_data->device_string = scriptengine->device_list_string();
    plot_data->test_name = scriptengine->test_name.toStdString();
    plot_data->plot = this;
    connect(export_button, &QPushButton::clicked,
            [plot = plot, plot_data = std::move(plot_data_up), export_button = export_button] { export_plot(plot, *plot_data, export_button); });
    assert(currently_in_gui_thread());
    clicker->setState(clicker->PointSelection);
    parent->add(plot, this);
    plot->setContextMenuPolicy(Qt::ContextMenuPolicy::PreventContextMenu);
    picker->setStateMachine(clicker);
    picker->setTrackerMode(QwtPicker::ActiveOnly);
    track_picker->setStateMachine(tracker);
    track_picker->setTrackerMode(QwtPicker::AlwaysOn);
    auto zoomer = new QwtPlotZoomer{plot->canvas()};
    zoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::MouseButton::LeftButton, Qt::ShiftModifier);
    plot->canvas()->installEventFilter(zoomer_controller = new Zoomer_controller(zoomer, this));
}

Plot::~Plot() {
    //Plot will be gone, but plot, zoomer, individual curves and so on will still exist and have to work, so they cannot depend on Plot or its members
    assert(currently_in_gui_thread());
    for (auto &curve : curves) {
        curve->detach();
        curve->plot = nullptr;
    }
    zoomer_controller->parent_plot = nullptr;
    plot_data->device_string = scriptengine->device_list_string();
    plot_data->plot = nullptr;
}

void Plot::clear() {
    assert(currently_in_gui_thread());
    for (auto &curve : curves) {
        curve->detach();
        curve->plot = nullptr;
    }
    curves.clear();
}

void Plot::set_x_marker(const std::string &title, double xpos, const Color &color) {
    assert(currently_in_gui_thread());
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
    assert(currently_in_gui_thread());
    plot->setVisible(visible);
}

void Plot::set_time_scale() {
    auto draw_engine = new QwtDateScaleEngine;
    plot->setAxisScaleEngine(QwtPlot::Axis::xBottom, draw_engine);
    plot->setAxisScaleDraw(QwtPlot::Axis::xBottom, new QwtDateScaleDraw);
    using_time_scale = zoomer_controller->using_time_scale = true;
    track_picker->scale_engine = draw_engine;
    plot->replot();
}

void Plot::set_export_text(std::string text) {
    plot_data->plot_text = text;
}

void Plot::update() {
    assert(currently_in_gui_thread());
    plot->replot();
}

void Plot::value_added(double x, double y) {
    zoomer_controller->value_added(x, y);
}

void Plot::curve_added(Curve *curve) {
    assert(currently_in_gui_thread());
    zoomer_controller->curves.push_back(curve->curve);
}
///\endcond
