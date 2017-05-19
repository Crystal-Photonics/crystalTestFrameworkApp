#include "pathsettingswindow.h"
#include "config.h"
#include "mainwindow.h"
#include "ui_pathsettingswindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QSettings>

PathSettingsWindow::PathSettingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PathSettingsWindow) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
    for (auto &conf : get_config_lines()) {
        conf.first->setText(QSettings{}.value(conf.second, "").toString());
    }
}

PathSettingsWindow::~PathSettingsWindow() {
    delete ui;
}

void PathSettingsWindow::on_settings_confirmation_accepted() {
    for (auto &conf : get_config_lines()) {
        QSettings{}.setValue(conf.second, conf.first->text());
    }
    close();
}

void PathSettingsWindow::on_settings_confirmation_rejected() {
    close();
}

std::vector<std::pair<QLineEdit *, const char *>> PathSettingsWindow::get_config_lines() const {
    return {{ui->isotope_db_path, Globals::isotope_source_data_base_path},
            {ui->test_script_path_text, Globals::test_script_path_settings_key},
            {ui->device_description_path_text, Globals::device_protocols_file_settings_key},
            {ui->rpc_xml_files_path_text, Globals::rpc_xml_files_path_settings_key},
            {ui->lua_editor_path_text, Globals::lua_editor_path_settings_key},
            {ui->lua_editor_parameters_text, Globals::lua_editor_parameters_settings_key},
            {ui->meta_path_text, Globals::measurement_equipment_meta_data_path},
            {ui->forms_path_directory_text, Globals::form_directory},
            {ui->forms_definitions_path_directory_text, Globals::form_definitions_directory},
            {ui->git_path, Globals::git_path},
            {ui->env_var_path, Globals::path_to_environment_variables}

    };
}

static void request_user_dir(QLineEdit *text, QString title, const char *key) {
    const auto selected_dir = QFileDialog::getExistingDirectory(MainWindow::mw, title, QSettings{}.value(key, "").toString(),
                                                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!selected_dir.isEmpty()) {
        text->setText(selected_dir);
    }
}

static void request_user_file(QLineEdit *text, QString title, const char *key, const char *file_extension) {
    const auto selected_file = QFileDialog::getOpenFileName(MainWindow::mw, title, QSettings{}.value(key, "").toString(), file_extension);
    if (!selected_file.isEmpty()) {
        text->setText(selected_file);
    }
}

void PathSettingsWindow::on_test_script_path_selector_clicked() {
    request_user_dir(ui->test_script_path_text, tr("Select test script path"), Globals::test_script_path_settings_key);
}

void PathSettingsWindow::on_device_description_path_selector_clicked() {
    request_user_file(ui->device_description_path_text, tr("Select device protocols description file"), Globals::device_protocols_file_settings_key, "*.json");
}

void PathSettingsWindow::on_rpc_xml_files_path_selector_clicked() {
    request_user_dir(ui->rpc_xml_files_path_text, tr("Select RPC-xml path"), Globals::rpc_xml_files_path_settings_key);
}

void PathSettingsWindow::on_lua_editor_path_selector_clicked() {
    request_user_file(ui->lua_editor_path_text, tr("Select lua editor executable"), Globals::lua_editor_path_settings_key, "*.exe");
}

void PathSettingsWindow::on_meta_path_selector_clicked() {
    request_user_file(ui->meta_path_text, tr("Select measurement equipment meta data file"), Globals::measurement_equipment_meta_data_path, "*.json");
}

void PathSettingsWindow::on_forms_path_directory_selector_clicked() {
    request_user_dir(ui->forms_path_directory_text, tr("Select directory for XML-forms layout files"), Globals::form_directory);
}

void PathSettingsWindow::on_forms_definitions_path_directory_selector_clicked() {
    request_user_dir(ui->forms_definitions_path_directory_text, tr("Select directory for JSON-forms definition files"), Globals::form_definitions_directory);
}

void PathSettingsWindow::on_git_path_selector_clicked() {
    request_user_file(ui->git_path, tr("Select the location of the git executable"), Globals::git_path, "git.exe");
}

void PathSettingsWindow::on_env_var_path_button_clicked() {
    request_user_file(ui->env_var_path, tr("Select the location of the environment variable file"), Globals::path_to_environment_variables, "*.json");
}
