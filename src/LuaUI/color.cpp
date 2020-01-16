#include "color.h"

#include <QColor>
#include <QString>
#include <map>

/// \cond HIDDEN_SYMBOLS
Color::Color(unsigned int rgb)
    : rgb(rgb) {}

std::string Color::name() const {
    static const auto rgb_to_name = [] {
        std::map<QRgb, std::string> colormap;
        for (const auto &name : QColor::colorNames()) {
            colormap.insert({QColor{name}.rgb(), name.toStdString()});
        }
        return colormap;
    }();
    if (const auto it = rgb_to_name.find(rgb); it != std::end(rgb_to_name)) {
        return it->second;
    }
    return QColor{rgb}.name().toStdString();
}

Color::Color(const std::string &name) {
    QColor color{name.c_str()};
    if (not color.isValid()) {
        throw std::runtime_error{"Invalid color: \"" + name +
                                 "\". Check https://www.w3.org/wiki/CSS/Properties/color/keywords#Extended_colors for a list of valid color names."};
    }
    rgb = color.rgb();
}

Color::Color(unsigned int r, unsigned int g, unsigned int b)
    : rgb{(r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF)} {}
/// \endcond
