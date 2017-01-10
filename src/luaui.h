#ifndef LUAUI_H
#define LUAUI_H

#include <QAction>
#include <QObject>
#include <atomic>
#include <memory>
#include <vector>

class QwtPlot;
class QwtPlotCurve;
class QSplitter;
class MainWindow;

class LuaPlot {
	public:
	LuaPlot(MainWindow *mw, QSplitter *splitter);
	void add(double x, double y);
	void clear();

	private:
	int id = -1;
	MainWindow *mw = nullptr;
	static std::atomic<int> current_plot_id;
};

struct LuaUI {
	LuaUI(QSplitter *parent);
	LuaPlot create_plot() const;
	static MainWindow *mw;

	private:
	QSplitter *parent = nullptr;
};

#endif // LUAUI_H
