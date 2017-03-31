#ifndef LABEL_H
#define LABEL_H


#include <QLabel>
#include <QSplitter>

class Label {
    public:
    ///\cond HIDDEN_SYMBOLS
    Label(QSplitter *parent, const std::string text);
    ~Label();
    ///\endcond
    void set_text(const std::string text);
    std::string get_text() const;

    private:
    QLabel *label = nullptr;
};

#endif // LABEL_H
