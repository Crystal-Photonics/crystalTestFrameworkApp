#include "luaui.h"
#include "mainwindow.h"

#include <QDebug>

std::atomic<int> LuaButton::current_id{0};

LuaUI::LuaUI(QSplitter *parent)
	: parent(parent) {}

LuaButton LuaUI::create_button(const std::string &title) const {
	return {parent, title};
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
