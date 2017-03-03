#include "plot.h"
#include "config.h"

#include <QAction>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <fstream>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>

Curve::Curve(QSplitter *, Plot *plot)
	: plot(plot)
	, curve(new QwtPlotCurve) {
	curve->attach(plot->plot);
	curve->setTitle("curve" + QString::number(plot->curve_id_counter++));
	plot->curves.push_back(this);
	plot->set_rightclick_action();
	callback_connection = QObject::connect(plot->picker, static_cast<void (QwtPlotPicker::*)(const QPointF &)>(&QwtPlotPicker::selected),
										   [this](const QPointF &selection) { selected_event(selection); });
}

Curve::~Curve() {
	if (plot) {
		auto &curves = plot->curves;
		curves.erase(std::find(std::begin(curves), std::end(curves), this));
		detach();
	}
	QObject::disconnect(callback_connection);
}

void Curve::add(double x, double y) {
	xvalues.push_back(x);
	yvalues_orig.push_back(y);
	update();
}

void Curve::add(const std::vector<double> &data) {
	resize(data.size());
	std::transform(std::begin(data), std::end(data), std::begin(yvalues_orig), std::begin(yvalues_orig), std::plus<>());
	update();
}

void Curve::add(const unsigned int spectrum_start_channel, const std::vector<double> &data) {
	size_t s = std::max(xvalues.size(), data.size() + spectrum_start_channel);
	resize(s);
	std::transform(std::begin(data), std::end(data), std::begin(yvalues_orig) + spectrum_start_channel, std::begin(yvalues_orig) + spectrum_start_channel,
				   std::plus<>());
	update();
}

void Curve::clear() {
	xvalues.clear();
	yvalues_orig.clear();
	update();
}

void Curve::set_offset(double offset) {
	this->offset = offset;
	xvalues.clear();
	resize(yvalues_orig.size());
}

void Curve::set_gain(double gain) {
	this->gain = gain;
	xvalues.clear();
	resize(yvalues_orig.size());
	resize(yvalues_plot.size());
}

double Curve::integrate_ci(double integral_start_ci, double integral_end_ci) {
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

void Curve::set_enable_median(bool enable) {
	median_enable = enable;
}

void Curve::set_median_kernel_size(int kernel_size) {
	if (kernel_size & 1) {
		median_kernel_size = kernel_size;
	} else {
		//TODO: Tell the user that the call had no effect?
	}
}

void Curve::set_color(const QColor &color) {
	curve->setPen(color);
}

void Curve::set_color_by_name(const std::string &name) {
	set_color(QString::fromStdString(name));
}

void Curve::set_color_by_rgb(int r, int g, int b) {
	set_color(QColor{r, g, b});
}

void Curve::set_click_callback(std::function<void(double, double)> click_callback) {
	this->click_callback = std::move(click_callback);
}

void Curve::resize(std::size_t size) {
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

void Curve::update() {
	if (median_enable && (median_kernel_size < yvalues_orig.size())) {
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
	curve->setRawSamples(xvalues.data(), yvalues_plot.data(), xvalues.size());
	plot->update();
}

void Curve::detach() {
	curve->setSamples(xvalues.data(), yvalues_plot.data(), xvalues.size());
}

void Curve::selected_event(const QPointF &selection) {
	click_callback(selection.x(), selection.y());
}

Plot::Plot(QSplitter *parent)
	: plot(new QwtPlot)
	, picker(new QwtPlotPicker(plot->canvas()))
	, clicker(new QwtPickerClickPointMachine) {
	clicker->setState(clicker->PointSelection);
    parent->addWidget(plot);
    plot->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
	set_rightclick_action();
	picker->setStateMachine(clicker);
	picker->setTrackerMode(QwtPicker::ActiveOnly);
}

Plot::~Plot() {
	for (auto &curve : curves) {
		curve->detach();
		curve->plot = nullptr;
	}
    //the plot was using xvalues and yvalues directly, but now they are gone
    //this is to make the plot own the data
}

void Plot::clear() {
	curves.clear();
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
			QString last_dir = QSettings{}.value(Globals::last_csv_saved_directory, QDir::currentPath()).toString();
			auto dir = QFileDialog::getExistingDirectory(plot, QObject::tr("Select folder to save data in"), last_dir);
			if (dir.isEmpty() == false) {
				QSettings{}.setValue(Globals::last_csv_saved_directory, dir);
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
