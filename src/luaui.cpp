#include "luaui.h"

#include <QAction>
#include <QFileDialog>
#include <fstream>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

LuaUI::Plot LuaUI::create_plot() {
	return {parent};
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

LuaUI::Plot::Plot(QSplitter *parent)
	: plot(new QwtPlot(parent))
	, curve(new QwtPlotCurve)
	, save_as_csv_action(std::make_unique<QAction>())
	, xvalues(std::make_unique<std::vector<double>>())
	, yvalues(std::make_unique<std::vector<double>>()) {
	parent->addWidget(plot);
	curve->attach(plot);
	plot->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);

	save_as_csv_action->setText(QObject::tr("save_as_csv"));
	QObject::connect(save_as_csv_action.get(), &QAction::triggered, [ plot = plot, &xvalues = *xvalues, &yvalues = *yvalues ] {
		auto file = QFileDialog::getSaveFileName(plot, QObject::tr("Select File to save data in"), "date.csv", "*.csv");
		if (file.isEmpty() == false) {
			std::ofstream f{file.toStdString()};
			for (std::size_t i = 0; i < xvalues.size(); i++) {
				f << xvalues[i] << ',' << yvalues[i] << '\n';
			}
		}
	});
	plot->addAction(save_as_csv_action.get());
}

void LuaUI::Plot::add(double x, double y) {
	xvalues->push_back(x);
	yvalues->push_back(y);
	update();
}

void LuaUI::Plot::clear() {
	xvalues->clear();
	yvalues->clear();
	update();
}

void LuaUI::Plot::update() {
	curve->setRawSamples(xvalues->data(), yvalues->data(), xvalues->size());
	plot->replot();
}
