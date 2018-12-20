#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include "userentrystorage.h"
#include <QScrollArea>
#include <vector>

class QVBoxLayout;
class QResizeEvent;
class QWidget;
class QLayout;
struct Widget_paragraph;
class UI_container;

class UI_widget {
    public:
    UI_widget(UI_container *parent_);
    ~UI_widget();

    virtual void resizeMe(QResizeEvent *event);

    UI_container *parent;
};

class UI_container : public QScrollArea {
    public:
    UI_container(QWidget *parent = nullptr);
    ~UI_container();
    void add(QWidget *widget, UI_widget *lua_ui_widget);
    void add(QLayout *layout, UI_widget *lua_ui_widget);
    void set_column_count(int columns);
    void scroll_to_bottom();
    void remove_me_from_resize_list(UI_widget *me);
    // UserEntryCache user_entry_cache;

    private:
    void resizeEvent(QResizeEvent *event) override;
    int compute_size(int width);
    void trigger_resize();

    QVBoxLayout *layout{nullptr};
    std::vector<Widget_paragraph> paragraphs;
    int column_count{1};
};

#endif // UI_CONTAINER_H
