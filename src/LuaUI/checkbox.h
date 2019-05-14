#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

#include "ui_container.h"

class CheckBox : public UI_widget {
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
    void set_enabled(bool enable);

    private:
    QCheckBox *checkbox = nullptr;
};

#endif // CHECKBOX_H
