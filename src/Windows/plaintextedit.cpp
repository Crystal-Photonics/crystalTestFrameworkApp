#include "plaintextedit.h"
#include "mainwindow.h"

PlainTextEdit::PlainTextEdit(QWidget *parent)
	: QPlainTextEdit(parent) {
	setTextInteractionFlags(Qt::TextBrowserInteraction);
	QObject::connect(this, &PlainTextEdit::linkActivated, MainWindow::mw, &MainWindow::link_activated);
}

void PlainTextEdit::mousePressEvent(QMouseEvent *e) {
	clickedAnchor = (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) : QString();
	QPlainTextEdit::mousePressEvent(e);
}

void PlainTextEdit::mouseReleaseEvent(QMouseEvent *e) {
	if (e->button() & Qt::LeftButton && !clickedAnchor.isEmpty() && anchorAt(e->pos()) == clickedAnchor) {
		emit linkActivated(clickedAnchor);
	}
	QPlainTextEdit::mouseReleaseEvent(e);
}
