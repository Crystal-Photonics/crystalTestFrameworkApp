#ifndef PLOT_H
#define PLOT_H

#include <QAction>
#include <vector>
#include <memory>
#include <qwt_plot.h>

//class QwtPlot;
class QSplitter;
class QwtPlotCurve;

class Plot {
	public:
	Plot(QSplitter *parent);
	void add(double x, double y);
	void add(const std::vector<int> &data);
	void clear();
	std::unique_ptr<QwtPlot> plot;
	QwtPlotCurve *curve = nullptr;
	QAction save_as_csv_action;

	std::vector<double> xvalues;
	std::vector<double> yvalues;

	private:
	void update();
	void resize(std::size_t size);
};


#endif // PLOT_H
