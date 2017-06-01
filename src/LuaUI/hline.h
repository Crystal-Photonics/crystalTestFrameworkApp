#ifndef HLINE_H
#define HLINE_H

#include "ui_container.h"

class QFrame;
class QWidget;

class HLine : public UI_widget{
    public:
    ///\cond HIDDEN_SYMBOLS
    HLine(UI_container *parent);
    ~HLine();
    ///\endcond

    void set_visible(bool visible);
private:
    QFrame *line = nullptr;
};

#endif // HLINE_H
