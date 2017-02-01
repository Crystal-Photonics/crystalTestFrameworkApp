#include "pathsettingswindow.h"
#include "config.h"
#include "ui_pathsettingswindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QSettings>

PathSettingsWindow::PathSettingsWindow(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::PathSettingsWindow) {
	ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
	ui->test_script_path_text->setText(QSettings{}.value(Globals::test_script_path_settings_key, "").toString());
	ui->device_description_path_text->setText(QSettings{}.value(Globals::device_protocols_file_settings_key, "").toString());
	ui->rpc_xml_files_path_text->setText(QSettings{}.value(Globals::rpc_xml_files_path_settings_key, "").toString());
	ui->lua_editor_path_text->setText(QSettings{}.value(Globals::lua_editor_path_settings_key, "").toString());
	ui->lua_editor_parameters_text->setText(QSettings{}.value(Globals::lua_editor_parameters_settings_key, "").toString());
}

PathSettingsWindow::~PathSettingsWindow() {
	delete ui;
}

void PathSettingsWindow::on_settings_confirmation_accepted() {
	QSettings{}.setValue(Globals::test_script_path_settings_key, ui->test_script_path_text->text());
	QSettings{}.setValue(Globals::device_protocols_file_settings_key, ui->device_description_path_text->text());
	QSettings{}.setValue(Globals::rpc_xml_files_path_settings_key, ui->rpc_xml_files_path_text->text());
	QSettings{}.setValue(Globals::lua_editor_path_settings_key, ui->lua_editor_path_text->text());
	QSettings{}.setValue(Globals::lua_editor_parameters_settings_key, ui->lua_editor_parameters_text->text());
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

void PathSettingsWindow::on_device_description_path_selector_clicked() {
	const auto selected_file = QFileDialog::getOpenFileName(this, tr("Select device description path"), "");
	if (!selected_file.isEmpty()) {
		ui->device_description_path_text->setText(selected_file);
	}
}

void PathSettingsWindow::on_rpc_xml_files_path_selector_clicked()
{
	const auto selected_dir =
		QFileDialog::getExistingDirectory(this, tr("Select RPC-xml path"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!selected_dir.isEmpty()) {
		ui->rpc_xml_files_path_text->setText(selected_dir);
	}
}

void PathSettingsWindow::on_lua_editor_path_selector_clicked()
{
	const auto selected_file = QFileDialog::getOpenFileName(this, tr("Select device description path"), "*.exe");
	if (!selected_file.isEmpty()) {
		ui->lua_editor_path_text->setText(selected_file);
	}
}
