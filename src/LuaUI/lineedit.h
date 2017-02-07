#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <string>

class QSplitter;
class QLineEdit;

class LineEdit {
	public:
	LineEdit(QSplitter *parent);
	~LineEdit();
	void set_placeholder_text(const std::string &text);
	std::string get_text() const;

	private:
	QLineEdit *edit;
};

#endif // LINEEDIT_H
