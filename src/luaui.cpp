#include "luaui.h"
#include "mainwindow.h"

#include <QDebug>

std::atomic<int> LuaPlot::current_plot_id{0};

LuaPlot::LuaPlot(QSplitter *splitter)
	: id(current_plot_id++) {
	MainWindow::mw->create_plot(id, splitter);
}

LuaPlot::LuaPlot(LuaPlot &&other)
	: id(-1) {
	std::swap(id, other.id);
}

LuaPlot &LuaPlot::operator=(LuaPlot &&other) {
	std::swap(id, other.id);
	return *this;
}

LuaPlot::~LuaPlot()
{
	MainWindow::mw->drop_plot(id);
}

void LuaPlot::add(double x, double y) {
	MainWindow::mw->add_data_to_plot(id, x, y);
}

void LuaPlot::clear() {
	MainWindow::mw->clear_plot(id);
}

LuaPlot LuaUI::create_plot() const {
	return {parent};
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}
