#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

class UI_container;

class CheckBox {
    public:
    ///\cond HIDDEN_SYMBOLS
	CheckBox(UI_container *parent);
    ~CheckBox();
    ///\endcond
    void set_checked(const bool checked);
    bool get_checked() const;
    void set_text(const std::string text);
    std::string get_text() const;

    private:
    QCheckBox *checkbox = nullptr;
    QWidget *base_widget = nullptr;
};

#endif // CHECKBOX_H
