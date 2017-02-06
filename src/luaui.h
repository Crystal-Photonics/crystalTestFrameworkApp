#ifndef LUAUI_H
#define LUAUI_H

#include <atomic>
#include <memory>
#include <string>
#include <vector>

class MainWindow;
class QSplitter;
class QwtPlot;
class QwtPlotCurve;

class LuaButton {
	public:
	LuaButton(QSplitter *splitter, const std::string &title);
	LuaButton(LuaButton &&other);
	LuaButton &operator=(LuaButton &&other);
	~LuaButton();
	bool has_been_pressed() const;

	private:
	int id = -1;
	std::unique_ptr<std::atomic<bool>> pressed = std::make_unique<std::atomic<bool>>(false);
	static std::atomic<int> current_id;
};

struct LuaUI {
	LuaUI(QSplitter *parent);
	LuaButton create_button(const std::string &title) const;

	QSplitter *parent = nullptr;
	private:
};

#endif // LUAUI_H
