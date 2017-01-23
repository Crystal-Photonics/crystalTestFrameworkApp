#include "luaui.h"
#include "mainwindow.h"

#include <QDebug>

std::atomic<int> LuaPlot::current_id{0};

LuaPlot::LuaPlot(QSplitter *splitter)
	: id(current_id++) {
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

LuaPlot::~LuaPlot() {
	if (id != -1) {
		MainWindow::mw->drop_plot(id);
	}
}

void LuaPlot::add_point(double x, double y) {
	MainWindow::mw->add_data_to_plot(id, x, y);
}

void LuaPlot::add_spectrum(const std::vector<double> &data) {
	MainWindow::mw->add_data_to_plot(id, data);
}

void LuaPlot::clear() {
	MainWindow::mw->clear_plot(id);
}

void LuaPlot::set_offset(double offset)
{
	MainWindow::mw->set_offset(id, offset);
}

void LuaPlot::set_gain(double gain)
{
	MainWindow::mw->set_gain(id, gain);
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

LuaPlot LuaUI::create_plot() const {
	return {parent};
}
