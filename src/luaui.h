#ifndef LUAUI_H
#define LUAUI_H

#include <QAction>
#include <QObject>
#include <atomic>
#include <memory>
#include <string>
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
    void add_spectrum(const unsigned int spectrum_start_channel, std::vector<double> &data);
	void clear();
	void set_offset(double offset);
	void set_gain(double gain);

	private:
	int id = -1;
	static std::atomic<int> current_id;
};

class LuaButton {
	public:
	LuaButton(QSplitter *splitter, const std::string &title);
	LuaButton(LuaButton &&other);
	LuaButton &operator=(LuaButton &&other);
	~LuaButton();
	bool has_been_pressed() const;

	private:
	int id = -1;
	std::unique_ptr<bool> pressed = std::make_unique<bool>(false);
	static std::atomic<int> current_id;
};

struct LuaUI {
	LuaUI(QSplitter *parent);
	LuaPlot create_plot() const;
	LuaButton create_button(const std::string &title) const;

	private:
	QSplitter *parent = nullptr;
};

#endif // LUAUI_H
