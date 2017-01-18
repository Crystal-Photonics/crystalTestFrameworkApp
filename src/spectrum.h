#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <QAction>
#include <memory>
#include <qwt_plot.h>
#include <vector>

class QSplitter;
class QwtPlotCurve;

class Spectrum {
	public:
	Spectrum(QSplitter *parent);
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

#endif // SPECTRUM_H
