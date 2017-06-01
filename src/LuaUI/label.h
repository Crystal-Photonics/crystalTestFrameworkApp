#ifndef LABEL_H
#define LABEL_H

#include <string>
#include "ui_container.h"

class QLabel;
class QWidget;


class Label : public UI_widget{
    public:
    ///\cond HIDDEN_SYMBOLS
	Label(UI_container *parent, const std::string text);
    ~Label();
    ///\endcond
    void set_text(const std::string text);
    std::string get_text() const;

    void set_visible(bool visible);
    void set_font_size(bool big_font);
private:
    QLabel *label = nullptr;
    int normal_font_size;
};

#endif // LABEL_H
