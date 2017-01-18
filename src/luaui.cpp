#include "luaui.h"
#include "mainwindow.h"

#include <QDebug>

std::atomic<int> LuaPlot::current_id{0};
std::atomic<int> LuaSpectrum::current_id{0};

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

void LuaPlot::add(double x, double y) {
	MainWindow::mw->add_data_to_plot(id, x, y);
}

void LuaPlot::clear() {
	MainWindow::mw->clear_plot(id);
}

LuaPlot LuaUI::create_plot() const {
	return {parent};
}

LuaSpectrum LuaUI::create_spectrum() const {
	return {parent};
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

LuaSpectrum::LuaSpectrum(QSplitter *splitter)
	: id(current_id++) {
	MainWindow::mw->create_spectrum(id, splitter);
}

LuaSpectrum::LuaSpectrum(LuaSpectrum &&other)
	: id(-1) {
	std::swap(id, other.id);
}

LuaSpectrum &LuaSpectrum::operator=(LuaSpectrum &&other) {
	std::swap(id, other.id);
	return *this;
}

LuaSpectrum::~LuaSpectrum() {
	if (id != -1) {
		MainWindow::mw->drop_spectrum(id);
	}
}

void LuaSpectrum::add(const std::vector<int> &data) {
	MainWindow::mw->add_data_to_spectrum(id, data);
}

void LuaSpectrum::clear() {
	MainWindow::mw->clear_spectrum(id);
}
