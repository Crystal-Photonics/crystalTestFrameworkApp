#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>
#include <QLabel>
#include <sol.hpp>

#include "ui_container.h"

class ComboBox : public UI_widget {
    public:
    ///\cond HIDDEN_SYMBOLS
    ComboBox(UI_container *parent, const QStringList &sl);
    ~ComboBox();
    ///\endcond
    void set_items(const sol::table &items);
    std::string get_text() const;
    void set_index(unsigned int index);
    unsigned int get_index();

    void set_caption(const std::string caption);
    void set_name(const std::string name);
    std::string get_caption() const;

    void set_editable(bool editable);

    void set_visible(bool visible);

    private:
    QString name_m;
    QComboBox *combobox = nullptr;
    QLabel *label = nullptr;
};

#endif // COMBOBOX_H
