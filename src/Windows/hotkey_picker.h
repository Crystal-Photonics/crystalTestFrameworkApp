#ifndef HOTKEY_PICKER_H
#define HOTKEY_PICKER_H

#include <QDialog>

#include <tuple>

class QKeySequenceEdit;

namespace Ui {
	class Hotkey_picker;
}

class Hotkey_picker : public QDialog {
	Q_OBJECT

	public:
	explicit Hotkey_picker(QWidget *parent = 0);
	~Hotkey_picker();

	private slots:

	void on_buttonBox_accepted();

	void on_buttonBox_rejected();

	private:
	Ui::Hotkey_picker *ui;

	void load();
	void save() const;

	struct Key_config {
		QKeySequenceEdit *edit_field{};
		const char *config{};
		const char *default_key{};
	};

	std::vector<Key_config> get_key_sequence_config() const;
};

#endif // HOTKEY_PICKER_H
