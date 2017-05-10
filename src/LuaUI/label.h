#ifndef LABEL_H
#define LABEL_H

#include <string>

class QLabel;
class QWidget;
class UI_container;

class Label {
    public:
    ///\cond HIDDEN_SYMBOLS
	Label(UI_container *parent, const std::string text);
    ~Label();
    ///\endcond
    void set_text(const std::string text);
    std::string get_text() const;

    void set_visible(bool visible);
private:
    QLabel *label = nullptr;
};

#endif // LABEL_H
