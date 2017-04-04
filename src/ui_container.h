#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include <QScrollArea>

class QVBoxLayout;

class UI_container : public QScrollArea
{
	public:
	UI_container(QWidget *parent = nullptr);
	void add_right(QWidget *widget);
	void add_below(QWidget *widget);
	private:
	QVBoxLayout *layout;
};

#endif // UI_CONTAINER_H
