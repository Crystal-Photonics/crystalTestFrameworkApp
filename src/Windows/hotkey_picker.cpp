#include "hotkey_picker.h"
#include "config.h"
#include "ui_hotkey_picker.h"

#include <QSettings>

Hotkey_picker::Hotkey_picker(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::Hotkey_picker) {
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
	load();
}

Hotkey_picker::~Hotkey_picker() {
	delete ui;
}

void Hotkey_picker::on_buttonBox_accepted() {
	save();
	close();
}

void Hotkey_picker::on_buttonBox_rejected() {
	close();
}

void Hotkey_picker::load() {
	for (const auto &ks : get_key_sequence_config()) {
		ks.first->setKeySequence(QKeySequence::fromString(QSettings{}.value(ks.second, "").toString()));
	}
}

void Hotkey_picker::save() const {
	for (const auto &ks : get_key_sequence_config()) {
		QSettings{}.setValue(ks.second, ks.first->keySequence().toString(QKeySequence::PortableText));
	}
}

std::vector<std::pair<QKeySequenceEdit *, const char *>> Hotkey_picker::get_key_sequence_config() const {
	return {
		{ui->confirm_keySequenceEdit, Globals::confirm_key_sequence},
		{ui->skip_keySequenceEdit, Globals::skip_key_sequence},
		{ui->cancel_keySequenceEdit, Globals::cancel_key_sequence},
	};
}
