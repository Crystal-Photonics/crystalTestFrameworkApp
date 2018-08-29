#include "pathsettingswindow.h"
#include "config.h"
#include "lua_functions.h"
#include "mainwindow.h"
#include "ui_pathsettingswindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include <QToolTip>

PathSettingsWindow::PathSettingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PathSettingsWindow) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
    QSettings q_settings{};
    load_ui_from_settings(q_settings);
}

PathSettingsWindow::~PathSettingsWindow() {
    delete ui;
}

void PathSettingsWindow::write_ui_to_settings(QSettings &q_settings) {
    for (auto &conf : get_config_lines()) {
        q_settings.setValue(conf.second, conf.first->text());
    }
}

void PathSettingsWindow::load_ui_from_settings(QSettings &q_settings) {
    for (auto &conf : get_config_lines()) {
        conf.first->setText(q_settings.value(conf.second, "").toString());
    }
}

void PathSettingsWindow::on_settings_confirmation_accepted() {
    QSettings q_settigns{};
    write_ui_to_settings(q_settigns);
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
            // {ui->git_path, Globals::git_path},
            {ui->env_var_path, Globals::path_to_environment_variables},
            {ui->edit_exceptional_approval, Globals::path_to_excpetional_approval_db},
            {ui->search_path, Globals::search_path}

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

void PathSettingsWindow::on_env_var_path_button_clicked() {
    request_user_file(ui->env_var_path, tr("Select the location of the environment variable file"), Globals::path_to_environment_variables, "*.json");
}

void PathSettingsWindow::on_exceptional_approval_path_clicked() {
    request_user_file(ui->edit_exceptional_approval, tr("Select the location of the exceptional approval db file"), Globals::path_to_excpetional_approval_db,
                      "*.json");
}

void PathSettingsWindow::on_isotope_db_path_button_clicked() {
    request_user_file(ui->isotope_db_path, tr("Select Isotope Source Database"), Globals::isotope_source_data_base_path, "*.json");
}

void PathSettingsWindow::on_search_path_textChanged(const QString &arg1) {
    QStringList wrong_folders;
    QStringList sl = get_search_path_entries(arg1);
    for (QString s : sl) {
        if (QDir(s).exists() == false) {
            wrong_folders.append(s);
        }
    }
    if (wrong_folders.count()) {
        QString s = "Can not find folders:\n" + wrong_folders.join("\n");
        QToolTip::showText(ui->search_path->mapToGlobal(QPoint(0, 0)), s);
    } else {
        QToolTip::showText(ui->search_path->mapToGlobal(QPoint(0, 0)), "");
    }
}

void PathSettingsWindow::on_btn_save_to_file_clicked() {
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Config-Files (*.ini)"));
    if (dialog.exec()) {
        QString file_name = dialog.selectedFiles()[0];
        QSettings settings_file{file_name, QSettings::IniFormat};
        write_ui_to_settings(settings_file);
    }
}

void PathSettingsWindow::on_btn_load_from_file_clicked() {
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Config-Files (*.ini)"));
    if (dialog.exec()) {
        QString file_name = dialog.selectedFiles()[0];
        QSettings settings_file{file_name, QSettings::IniFormat};
        load_ui_from_settings(settings_file);
    }
}
