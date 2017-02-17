#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <string>
#include <functional>
#include <QMetaObject>

class QSplitter;
class QLineEdit;

class LineEdit {
	public:
	LineEdit(QSplitter *parent);
	~LineEdit();
	void set_placeholder_text(const std::string &text);
	std::string get_text() const;
    void set_name(const std::string &text);
    std::string get_name() const;
    double get_number() const;
	void set_single_shot_return_pressed_callback(std::function<void()> callback);

	private:
	QLineEdit *edit = nullptr;
    std::string name;
	QMetaObject::Connection callback_connection = {};
};

#endif // LINEEDIT_H
