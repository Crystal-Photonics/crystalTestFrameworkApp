#include "luaui.h"
#include "mainwindow.h"

#include <QAction>
#include <QFileDialog>
#include <fstream>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

MainWindow *LuaUI::mw = nullptr;

Plot LuaUI::create_plot() const {
	return {parent};
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

Plot::Plot(QSplitter *parent)
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

void Plot::add(double x, double y) {
	xvalues->push_back(x);
	yvalues->push_back(y);
	update();
}

void Plot::clear() {
	xvalues->clear();
	yvalues->clear();
	update();
}

void Plot::update() {
	//TODO: somehow do this with signals
	curve->setRawSamples(xvalues->data(), yvalues->data(), xvalues->size());
	plot->replot();
}
