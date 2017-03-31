#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>
#include <QLabel>
#include <QSplitter>
#include <sol.hpp>

class ComboBox {
    public:
    ///\cond HIDDEN_SYMBOLS
    ComboBox(QSplitter *parent);
    ~ComboBox();
    ///\endcond
    void set_items(sol::table items);
    std::string get_text() const;
    void set_index(unsigned int index);
    unsigned int get_index();

    void set_caption(const std::string caption);
    std::string get_caption() const;

    private:
    QComboBox *combobox = nullptr;
    QLabel *label = nullptr;
    QWidget *base_widget = nullptr;
};

#endif // COMBOBOX_H
