#include "color.h"
#include <QColor>

///\cond HIDDEN_SYMBOLS
Color::Color(int rgb)
    : rgb(rgb) {}
///\endcond

Color Color::Color_from_name(const std::string &name) {
    int c = QColor{name.c_str()}.rgb();
    return Color{c};
}

Color Color::Color_from_r_g_b(int r, int g, int b) {
    int c = (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF);
    return Color{c};
}

Color Color::Color_from_rgb(int rgb) {
    return Color{rgb};
}
