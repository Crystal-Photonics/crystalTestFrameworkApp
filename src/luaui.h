#ifndef LUAUI_H
#define LUAUI_H

#include <QAction>
#include <QSplitter>
#include <memory>
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
		std::unique_ptr<QAction> save_as_csv_action;

		std::unique_ptr<std::vector<double>> xvalues; //need to have pointers because Plot moves and pointers to xvalues and yvalues must stay valid
		std::unique_ptr<std::vector<double>> yvalues;

		private:
		void update();
	};

	private:
	QSplitter *parent;
};

#endif // LUAUI_H
