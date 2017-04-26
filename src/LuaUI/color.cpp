#include "color.h"
#include <QColor>

Color::Color(int rgb)
    : rgb(rgb) {}

Color::Color(const std::string &name)
	: rgb(QColor{name.c_str()}.rgb()) {}

Color::Color(int r, int g, int b)
	: rgb{(r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF)} {}
