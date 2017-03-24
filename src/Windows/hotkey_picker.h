#ifndef HOTKEY_PICKER_H
#define HOTKEY_PICKER_H

#include <QDialog>

namespace Ui {
	class Hotkey_picker;
}

class Hotkey_picker : public QDialog
{
	Q_OBJECT

	public:
	explicit Hotkey_picker(QWidget *parent = 0);
	~Hotkey_picker();

	private slots:
	void on_buttonBox_rejected();

	void on_buttonBox_accepted();

	private:
	Ui::Hotkey_picker *ui;
};

#endif // HOTKEY_PICKER_H
