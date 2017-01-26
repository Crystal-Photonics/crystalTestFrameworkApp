#include "luaui.h"
#include "mainwindow.h"

#include <QDebug>

std::atomic<int> LuaPlot::current_id{0};
std::atomic<int> LuaButton::current_id{0};

LuaPlot::LuaPlot(QSplitter *splitter)
	: id(current_id++) {
	MainWindow::mw->plot_create(id, splitter);
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
		MainWindow::mw->plot_drop(id);
	}
}

void LuaPlot::add_point(double x, double y) {
	MainWindow::mw->plot_add_data(id, x, y);
}

void LuaPlot::add_spectrum(const std::vector<double> &data) {
	MainWindow::mw->plot_add_data(id, data);
}

void LuaPlot::clear() {
	MainWindow::mw->plot_clear(id);
}

void LuaPlot::set_offset(double offset) {
	MainWindow::mw->plot_set_offset(id, offset);
}

void LuaPlot::set_gain(double gain) {
	MainWindow::mw->plot_set_gain(id, gain);
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

LuaPlot LuaUI::create_plot() const {
	return {parent};
}

LuaButton LuaUI::create_button(const std::string &title) const {
	return {parent, title};
}

void LuaUI::set_parent(QSplitter *parent) {
	qDebug() << "LuaUI parent changed from" << this->parent << "to" << parent;
	this->parent = parent;
}

LuaButton::LuaButton(QSplitter *splitter, const std::string &title)
	: id(current_id++) {
	MainWindow::mw->button_create(id, splitter, title, [&pressed = *pressed] { pressed = true; });
}

LuaButton::LuaButton(LuaButton &&other) {
	std::swap(id, other.id);
	std::swap(pressed, other.pressed);
}

LuaButton::~LuaButton() {
	if (id != -1) {
		MainWindow::mw->button_drop(id);
	}
}

bool LuaButton::has_been_pressed() const {
	return *pressed;
}
