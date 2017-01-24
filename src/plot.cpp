#include "plot.h"

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
	QObject::connect(save_as_csv_action, &QAction::triggered, [plot = this->plot, curve = this->curve] {
		auto file = QFileDialog::getSaveFileName(plot, QObject::tr("Select File to save data in"), "date.csv", "*.csv");
		if (file.isEmpty() == false) {
			std::ofstream f{file.toStdString()};
			auto data = curve->data();
			auto size = data->size();
			for (std::size_t i = 0; i < size; i++) {
				const auto &point = data->sample(i);
				f << point.x() << ',' << point.y() << '\n';
			}
		}
	});
	plot->addAction(save_as_csv_action);
}

Plot::~Plot() {
	curve->setSamples(xvalues.data(), yvalues.data(), xvalues.size());
	//the plot was using xvalues and yvalues directly, but now they are gone
	//this is to make the plot own the data
}

void Plot::add(double x, double y) {
	xvalues.push_back(x);
	yvalues.push_back(y);
	update();
}

void Plot::add(const std::vector<double> &data) {
	resize(data.size());
	std::transform(std::begin(data), std::end(data), std::begin(yvalues), std::begin(yvalues), std::plus<>());
	update();
}

void Plot::clear() {
	xvalues.clear();
	yvalues.clear();
	update();
}

void Plot::set_offset(double offset) {
	this->offset = offset;
	xvalues.clear();
	resize(yvalues.size());
}

void Plot::set_gain(double gain) {
	this->gain = gain;
	xvalues.clear();
	resize(yvalues.size());
}

void Plot::update() {
	curve->setRawSamples(xvalues.data(), yvalues.data(), xvalues.size());
	plot->replot();
}

void Plot::resize(std::size_t size) {
	if (xvalues.size() > size) {
		xvalues.resize(size);
		yvalues.resize(size);
	} else if (xvalues.size() < size) {
		auto old_size = xvalues.size();
		xvalues.resize(size);
		for (auto i = old_size; i < size; i++) {
			xvalues[i] = offset + gain * i;
		}
		yvalues.resize(size, 0.);
	}
}
