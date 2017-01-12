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
	LuaPlot(QSplitter *splitter);
	LuaPlot(LuaPlot &&other);
	LuaPlot &operator=(LuaPlot &&other);
	~LuaPlot();
	void add(double x, double y);
	void clear();

	private:
	int id = -1;
	static std::atomic<int> current_plot_id;
};

struct LuaUI {
	LuaUI(QSplitter *parent);
	LuaPlot create_plot() const;

	private:
	QSplitter *parent = nullptr;
};

#endif // LUAUI_H
