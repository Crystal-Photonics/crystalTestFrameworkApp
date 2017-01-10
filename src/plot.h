#ifndef PLOT_H
#define PLOT_H

#include <QAction>
#include <vector>

class QSplitter;
class QwtPlot;
class QwtPlotCurve;

class Plot {
	public:
	Plot(QSplitter *parent);
	void add(double x, double y);
	void clear();
	QwtPlot *plot = nullptr;
	QwtPlotCurve *curve = nullptr;
	QAction save_as_csv_action;

	std::vector<double> xvalues; //need to have pointers because Plot moves and pointers to xvalues and yvalues must stay valid
	std::vector<double> yvalues;

	private:
	void update();
};


#endif // PLOT_H
