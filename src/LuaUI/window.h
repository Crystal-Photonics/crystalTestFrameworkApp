#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class QSplitter;
class TestRunner;
///\cond HIDDEN_SYMBOLS
class Window : public QWidget {
    public:
	Window(TestRunner *test, QString title);
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;

    private:
    TestRunner *test;
};
///\endcond
#endif // WINDOW_H
