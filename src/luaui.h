#ifndef LUAUI_H
#define LUAUI_H

#include <QSplitter>
#include <vector>

class QwtPlot;
class QwtPlotCurve;

struct LuaUI {
	struct Plot;
	LuaUI(QSplitter *parent);
	Plot create_plot();
	struct Plot {
		Plot(QSplitter *parent);
		void add(double x, double y);
		void clear();
		QwtPlot *plot = nullptr;
		QwtPlotCurve *curve = nullptr;
		std::vector<double> xvalues;
		std::vector<double> yvalues;

		private:
		void update();
	};

	private:
	QSplitter *parent;
};

#endif // LUAUI_H
