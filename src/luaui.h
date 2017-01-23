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
	void add_point(double x, double y);
	void add_spectrum(const std::vector<double> &data);
	void clear();
	void set_offset(double offset);
	void set_gain(double gain);

	private:
	int id = -1;
	static std::atomic<int> current_id;
};

struct LuaUI {
	LuaUI(QSplitter *parent);
	LuaPlot create_plot() const;

	private:
	QSplitter *parent = nullptr;
};

#endif // LUAUI_H
