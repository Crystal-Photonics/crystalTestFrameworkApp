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
	static std::atomic<int> current_id;
};

class LuaSpectrum{
	public:
	LuaSpectrum(QSplitter *splitter);
	LuaSpectrum(LuaSpectrum &&other);
	LuaSpectrum &operator=(LuaSpectrum &&other);
	~LuaSpectrum();
	void add(const std::vector<int> &data);
	void clear();

	private:
	int id = -1;
	static std::atomic<int> current_id;
};

struct LuaUI {
	LuaUI(QSplitter *parent);
	LuaPlot create_plot() const;
	LuaSpectrum create_spectrum() const;

	private:
	QSplitter *parent = nullptr;
};

#endif // LUAUI_H
