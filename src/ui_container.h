#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include <QScrollArea>
#include <vector>

class QVBoxLayout;
class QResizeEvent;
class QWidget;
class QLayout;
struct Widget_paragraph;

class UI_container : public QScrollArea {
	public:
	UI_container(QWidget *parent = nullptr);
	~UI_container();
	void add(QWidget *widget);
	void add(QLayout *layout);
	void set_column_count(int columns);

	private:
	void resizeEvent(QResizeEvent *event) override;
	int compute_size(int width);
	void trigger_resize();

	QVBoxLayout *layout{nullptr};
	std::vector<Widget_paragraph> paragraphs;
	int column_count{1};
};

#endif // UI_CONTAINER_H
