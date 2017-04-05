#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include <QScrollArea>
#include <vector>

class QVBoxLayout;
class QResizeEvent;
class QWidget;

class UI_container : public QScrollArea
{
	public:
	UI_container(QWidget *parent = nullptr);
	void add_right(QWidget *widget);
	void add_below(QWidget *widget);
	private:
	void resizeEvent(QResizeEvent *event) override;
	int compute_size(int width);

	QVBoxLayout *layout;
	std::vector<QWidget *> widgets;
};

#endif // UI_CONTAINER_H
