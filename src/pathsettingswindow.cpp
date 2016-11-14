#include "pathsettingswindow.h"
#include "ui_pathsettingswindow.h"
#include "config.h"

#include <QDebug>
#include <QFileDialog>
#include <QSettings>

PathSettingsWindow::PathSettingsWindow(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::PathSettingsWindow) {
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
	ui->test_script_path_text->setText(QSettings{}.value(Globals::test_script_path_settings_key, "").toString());
}

PathSettingsWindow::~PathSettingsWindow() {
	delete ui;
}

void PathSettingsWindow::on_settings_confirmation_accepted() {
	QSettings{}.setValue(Globals::test_script_path_settings_key, ui->test_script_path_text->text());
	close();
}

void PathSettingsWindow::on_settings_confirmation_rejected() {
	close();
}

void PathSettingsWindow::on_test_script_path_selector_clicked() {
	const auto selected_dir =
		QFileDialog::getExistingDirectory(this, tr("Select test script path"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!selected_dir.isEmpty()) {
		ui->test_script_path_text->setText(selected_dir);
	}
}
