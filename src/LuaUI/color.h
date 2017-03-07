#ifndef COLOR_H
#define COLOR_H

#include <string>

struct Color {
	Color(int rgb);
	Color(int r, int g, int b);
	Color(const std::string &name);
	int rgb{};
};

#endif // COLOR_H
