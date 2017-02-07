#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class QSplitter;
class TestRunner;

class Window : public QWidget {
	public:
	Window(TestRunner *test);
	QSize sizeHint() const override;
	void closeEvent(QCloseEvent *event) override;
	private:
	TestRunner *test;
};

#endif // WINDOW_H
