#include "settingsform.h"
#include "ui_settingsform.h"

#include "LuaFunctions/lua_functions.h"
#include "config.h"
#include "mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include <QToolTip>

SettingsForm::SettingsForm(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsForm) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); //remove question mark from the title bar
    QSettings q_settings{};
    load_ui_from_settings(q_settings);
}

SettingsForm::~SettingsForm() {
    delete ui;
}

void SettingsForm::write_ui_to_settings(QSettings &q_settings) {
    for (auto &conf : get_config_lines()) {
        q_settings.setValue(conf.second, conf.first->text());
    }
    q_settings.setValue(Globals::collapse_script_explorer_on_scriptstart_key, ui->cbox_collapse_script_explorer_by_start->isChecked());
    q_settings.setValue(Globals::console_human_readable_view_key, ui->cbtn_console_human_readable->isChecked());
    for (const auto &ks : get_key_sequence_config()) {
        q_settings.setValue(ks.config, ks.edit_field->keySequence().toString(QKeySequence::PortableText));
    }
}

void SettingsForm::load_ui_from_settings(QSettings &q_settings) {
    for (auto &conf : get_config_lines()) {
        conf.first->setText(q_settings.value(conf.second, "").toString());
    }
    ui->cbox_collapse_script_explorer_by_start->setChecked(q_settings.value(Globals::collapse_script_explorer_on_scriptstart_key, false).toBool());
    ui->cbtn_console_human_readable->setChecked(q_settings.value(Globals::console_human_readable_view_key, false).toBool());
    for (const auto &ks : get_key_sequence_config()) {
        ks.edit_field->setKeySequence(QKeySequence::fromString(q_settings.value(ks.config, ks.default_key).toString()));
    }
}

void SettingsForm::on_btn_ok_clicked() {
    QSettings q_settigns{};
    write_ui_to_settings(q_settigns);
    close();
}

void SettingsForm::on_btn_cancel_clicked() {
    close();
}

std::vector<std::pair<QLineEdit *, const char *>> SettingsForm::get_config_lines() const {
    return {{ui->isotope_db_path, Globals::isotope_source_data_base_path_key},
            {ui->test_script_path_text, Globals::test_script_path_settings_key},
            {ui->device_description_path_text, Globals::device_protocols_file_settings_key},
            {ui->rpc_xml_files_path_text, Globals::rpc_xml_files_path_settings_key},
            {ui->lua_editor_path_text, Globals::lua_editor_path_settings_key},
            {ui->lua_editor_parameters_text, Globals::lua_editor_parameters_settings_key},
            {ui->meta_path_text, Globals::measurement_equipment_meta_data_path_key},
            {ui->env_var_path, Globals::path_to_environment_variables_key},
            {ui->luacheck_path, Globals::path_to_luacheck_key},
            {ui->edit_exceptional_approval, Globals::path_to_excpetional_approval_db_key},
            {ui->favorite_script_path, Globals::favorite_script_file_key},
            {ui->report_history_query, Globals::report_history_query_path},
            {ui->search_path, Globals::search_path_key},
            {ui->edit_libpaths, Globals::test_script_library_path_key}};
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

void SettingsForm::on_report_history_query_selector_clicked() {
    request_user_dir(ui->report_history_query, tr("Select query path"), Globals::report_history_query_path);
}

void SettingsForm::on_test_script_path_selector_clicked() {
    request_user_dir(ui->test_script_path_text, tr("Select test script path"), Globals::test_script_path_settings_key);
}

void SettingsForm::on_device_description_path_selector_clicked() {
    request_user_file(ui->device_description_path_text, tr("Select device protocols description file"), Globals::device_protocols_file_settings_key, "*.json");
}

void SettingsForm::on_rpc_xml_files_path_selector_clicked() {
    request_user_dir(ui->rpc_xml_files_path_text, tr("Select RPC-xml path"), Globals::rpc_xml_files_path_settings_key);
}

void SettingsForm::on_favorite_script_selector_clicked() {
    request_user_file(ui->favorite_script_path, tr("Select favorite script description file"), Globals::favorite_script_file_key, "*.json");
}

void SettingsForm::on_lua_editor_path_selector_clicked() {
    request_user_file(ui->lua_editor_path_text, tr("Select lua editor executable"), Globals::lua_editor_path_settings_key, "*.exe");
}

void SettingsForm::on_meta_path_selector_clicked() {
    request_user_file(ui->meta_path_text, tr("Select measurement equipment meta data file"), Globals::measurement_equipment_meta_data_path_key, "*.json");
}

void SettingsForm::on_env_var_path_button_clicked() {
    request_user_file(ui->env_var_path, tr("Select the location of the environment variable file"), Globals::path_to_environment_variables_key, "*.json");
}

void SettingsForm::on_exceptional_approval_path_clicked() {
    request_user_file(ui->edit_exceptional_approval, tr("Select the location of the exceptional approval db file"),
                      Globals::path_to_excpetional_approval_db_key, "*.json");
}

void SettingsForm::on_isotope_db_path_button_clicked() {
    request_user_file(ui->isotope_db_path, tr("Select Isotope Source Database"), Globals::isotope_source_data_base_path_key, "*.json");
}

void SettingsForm::on_search_path_textChanged(const QString &arg1) {
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

void SettingsForm::on_edit_libpaths_textChanged(const QString &arg1) {
    QStringList wrong_folders;
    QStringList sl = arg1.split(";");
    for (QString s : sl) {
        if (QDir(s).exists() == false) {
            wrong_folders.append(s);
        }
    }
    if (wrong_folders.count()) {
        QString s = "Can not find folders:\n" + wrong_folders.join("\n");
        QToolTip::showText(ui->edit_libpaths->mapToGlobal(QPoint(0, 0)), s);
    } else {
        QToolTip::showText(ui->edit_libpaths->mapToGlobal(QPoint(0, 0)), "");
    }
}

void SettingsForm::on_btn_export_clicked() {
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

void SettingsForm::on_btn_import_clicked() {
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

void SettingsForm::on_confirm_keySequenceEdit_clear_button_clicked() {
    ui->confirm_keySequenceEdit->clear();
}

void SettingsForm::on_skip_keySequenceEdit_clear_button_clicked() {
    ui->skip_keySequenceEdit->clear();
}

void SettingsForm::on_cancel_keySequenceEdit_clear_button_clicked() {
    ui->cancel_keySequenceEdit->clear();
}

std::vector<SettingsForm::Key_config> SettingsForm::get_key_sequence_config() const {
    return {
        {ui->confirm_keySequenceEdit, Globals::confirm_key_sequence_key, Globals::default_confirm_key_sequence_key},
        {ui->skip_keySequenceEdit, Globals::skip_key_sequence_key, Globals::default_skip_key_sequence_key},
        {ui->cancel_keySequenceEdit, Globals::cancel_key_sequence_key, Globals::default_cancel_key_sequence_key},
    };
}

void SettingsForm::on_luacheck_path_button_clicked() {
    request_user_file(ui->luacheck_path, tr("Select luacheck executable"), Globals::path_to_luacheck_key, "*.exe");
}
