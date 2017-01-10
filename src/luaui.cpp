#include "luaui.h"
#include "mainwindow.h"

#include <QDebug>

MainWindow *LuaUI::mw = nullptr;

std::atomic<int> LuaPlot::current_plot_id{0};

LuaPlot::LuaPlot(MainWindow *mw, QSplitter *splitter)
	: id(current_plot_id++)
	, mw(mw) {
	mw->create_plot(id, splitter);
}

void LuaPlot::add(double x, double y) {
	mw->add_data_to_plot(id, x, y);
}

void LuaPlot::clear() {
	mw->clear_plot(id);
}

LuaPlot LuaUI::create_plot() const {
	return {mw, parent};
}

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}
