#include "plot.h"

#include <QAction>
#include <QFileDialog>
#include <QSplitter>
#include <fstream>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

Plot::Plot(QSplitter *parent)
    : plot(new QwtPlot)
    , curve(new QwtPlotCurve)
    , save_as_csv_action(new QAction(plot)) {
    parent->addWidget(plot);
    curve->attach(plot);
    plot->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);

    save_as_csv_action->setText(QObject::tr("save_as_csv"));
    QObject::connect(save_as_csv_action, &QAction::triggered, [ plot = this->plot, curve = this->curve ] {
        auto file = QFileDialog::getSaveFileName(plot, QObject::tr("Select File to save data in"), "date.csv", "*.csv");
        if (file.isEmpty() == false) {
            std::ofstream f{file.toStdString()};
            auto data = curve->data();
            auto size = data->size();
            for (std::size_t i = 0; i < size; i++) {
                const auto &point = data->sample(i);
                f << point.x() << ';' << point.y() << '\n';
            }
        }
    });
    plot->addAction(save_as_csv_action);
}

Plot::~Plot() {
    curve->setSamples(xvalues.data(), yvalues_plot.data(), xvalues.size());
    //the plot was using xvalues and yvalues directly, but now they are gone
    //this is to make the plot own the data
}

void Plot::add(double x, double y) {
    xvalues.push_back(x);
    yvalues_orig.push_back(y);
    update();
}

void Plot::add(const std::vector<double> &data) {
    resize(data.size());
    std::transform(std::begin(data), std::end(data), std::begin(yvalues_orig), std::begin(yvalues_orig), std::plus<>());
    update();
}

void Plot::add(const unsigned int spectrum_start_channel, const std::vector<double> &data) {
    size_t s = std::max(xvalues.size(), data.size() + spectrum_start_channel);
    resize(s);
    std::transform(std::begin(data), std::end(data), std::begin(yvalues_orig) + spectrum_start_channel, std::begin(yvalues_orig) + spectrum_start_channel,
                   std::plus<>());
    update();
}

void Plot::clear() {
    xvalues.clear();
    yvalues_orig.clear();
    update();
}

void Plot::set_offset(double offset) {
    this->offset = offset;
    xvalues.clear();
    resize(yvalues_orig.size());
}

void Plot::set_enable_median(bool enable) {
    median_enable = enable;
}

void Plot::set_median_kernel_size(int kernel_size) {
    if (kernel_size & 1) {
        this->median_kernel_size = kernel_size;
    }
}

double Plot::integrate_ci(double integral_start_ci, double integral_end_ci) {
    double result = 0;
    if (integral_start_ci < 0) {
        integral_start_ci = 0;
    }

    if (integral_end_ci < 0) {
        integral_end_ci = 0;
    }
    unsigned int s = round(integral_start_ci);
    unsigned int e = round(integral_end_ci);

    if (s >= yvalues_plot.size()) {
        s = yvalues_plot.size() - 1;
    }

    if (e >= yvalues_plot.size()) {
        e = yvalues_plot.size() - 1;
    }

    for (unsigned int i = s; i < e; i++) {
        result += yvalues_plot[i];
    }
    return result;
}

void Plot::set_gain(double gain) {
    this->gain = gain;
    xvalues.clear();
    resize(yvalues_orig.size());
    resize(yvalues_plot.size());
}


void Plot::update() {
    if (median_enable && (median_kernel_size < yvalues_orig.size())) {
        std::vector<double> kernel(median_kernel_size);
        const unsigned int HALF_KERNEL_SIZE = median_kernel_size/2;

        for (unsigned int i = 0; i<  HALF_KERNEL_SIZE; i++){
            yvalues_plot[i] = yvalues_orig[i];
        }

        for (unsigned int i = yvalues_orig.size()-HALF_KERNEL_SIZE;  i < yvalues_orig.size(); i++){
            yvalues_plot[i] = yvalues_orig[i];
        }

        for (unsigned int i = HALF_KERNEL_SIZE; i< yvalues_orig.size()-HALF_KERNEL_SIZE; i++){
            for (unsigned int j = 0;j<median_kernel_size;j++){
                kernel[j] = yvalues_orig[i+j-HALF_KERNEL_SIZE];
            }
            std::sort(kernel.begin(),kernel.end());
            yvalues_plot[i] = kernel[HALF_KERNEL_SIZE];
        }

    } else {
        yvalues_plot = yvalues_orig;
    }
    curve->setRawSamples(xvalues.data(), yvalues_plot.data(), xvalues.size());
    plot->replot();
}

void Plot::resize(std::size_t size) {
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
    }
}
