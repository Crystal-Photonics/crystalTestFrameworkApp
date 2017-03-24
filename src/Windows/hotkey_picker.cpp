#include "hotkey_picker.h"
#include "ui_hotkey_picker.h"

Hotkey_picker::Hotkey_picker(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::Hotkey_picker) {
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
}

Hotkey_picker::~Hotkey_picker() {
	delete ui;
}

void Hotkey_picker::on_buttonBox_rejected() {
	close();
}

void Hotkey_picker::on_buttonBox_accepted() {
	close();
}
