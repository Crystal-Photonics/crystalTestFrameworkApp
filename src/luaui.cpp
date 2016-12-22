#include "luaui.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

LuaUI::Plot LuaUI::create_plot() {
	return {parent};
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

LuaUI::Plot::Plot(QSplitter *parent)
	: plot(new QwtPlot(parent))
	, curve(new QwtPlotCurve) {
	parent->addWidget(plot);
	curve->attach(plot);
}

void LuaUI::Plot::add(double x, double y) {
	xvalues.push_back(x);
	yvalues.push_back(y);
	update();
}

void LuaUI::Plot::clear() {
	xvalues.clear();
	yvalues.clear();
	update();
}

void LuaUI::Plot::update() {
	curve->setRawSamples(xvalues.data(), yvalues.data(), xvalues.size());
	plot->replot();
}
