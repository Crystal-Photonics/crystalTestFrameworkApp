#ifndef HLINE_H
#define HLINE_H


class QFrame;
class QWidget;
class UI_container;

class HLine {
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
