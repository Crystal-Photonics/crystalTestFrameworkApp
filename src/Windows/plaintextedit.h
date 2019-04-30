#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QMouseEvent>
#include <QPlainTextEdit>

//Adapted from Orest Hera, https://stackoverflow.com/a/33585505
class PlainTextEdit : public QPlainTextEdit {
	Q_OBJECT

	private:
	QString clickedAnchor;

	public:
	explicit PlainTextEdit(QWidget *parent = nullptr);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);

	signals:
	void linkActivated(QString);
};

#endif // PLAINTEXTEDIT_H
