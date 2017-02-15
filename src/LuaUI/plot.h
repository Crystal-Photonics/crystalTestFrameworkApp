#ifndef PLOT_H
#define PLOT_H

#include <memory>
#include <qwt_plot.h>
#include <vector>

class QAction;
class QSplitter;
class QwtPlot;
class QwtPlotCurve;

class Plot {
	public:
	Plot(QSplitter *parent);
	~Plot();
	void add(double x, double y);
	void add(const std::vector<double> &data);
	void add(const unsigned int spectrum_start_channel, const std::vector<double> &data);
	void clear();
    double integrate_ci(double integral_start_ci, double integral_end_ci);
	void set_offset(double offset);
	void set_gain(double gain);

	QwtPlot *plot = nullptr;
	QwtPlotCurve *curve = nullptr;
	QAction *save_as_csv_action = nullptr;

	private:
	void update();
	void resize(std::size_t size);
	std::vector<double> xvalues;
	std::vector<double> yvalues;
	double offset = 0;
	double gain = 1;
};

#endif // PLOT_H
