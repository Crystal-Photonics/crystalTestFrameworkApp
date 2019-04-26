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
	explicit PlainTextEdit(QWidget *parent = nullptr)
		: QPlainTextEdit(parent) {}

	void mousePressEvent(QMouseEvent *e) {
		clickedAnchor = (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) : QString();
		QPlainTextEdit::mousePressEvent(e);
	}

	void mouseReleaseEvent(QMouseEvent *e) {
		if (e->button() & Qt::LeftButton && !clickedAnchor.isEmpty() && anchorAt(e->pos()) == clickedAnchor) {
			emit linkActivated(clickedAnchor);
		}
		QPlainTextEdit::mouseReleaseEvent(e);
	}

	signals:
	void linkActivated(QString);
};

#endif // PLAINTEXTEDIT_H
