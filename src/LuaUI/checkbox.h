#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

class UI_container;

class CheckBox {
    public:
    ///\cond HIDDEN_SYMBOLS
    CheckBox(UI_container *parent, const std::string text);
    ~CheckBox();
    ///\endcond
    void set_checked(const bool checked);
    bool get_checked() const;
    void set_text(const std::string text);
    std::string get_text() const;

    void set_visible(bool visible);
private:
    QCheckBox *checkbox = nullptr;
};

#endif // CHECKBOX_H
