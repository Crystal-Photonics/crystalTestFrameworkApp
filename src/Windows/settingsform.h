#ifndef SETTINGSFORM_H
#define SETTINGSFORM_H

#include <QDialog>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QSettings>
#include <vector>

namespace Ui {
    class SettingsForm;
}

class SettingsForm : public QDialog {
    Q_OBJECT

    public:
    explicit SettingsForm(QWidget *parent = nullptr);
    ~SettingsForm();

    private slots:
    void on_test_script_path_selector_clicked();
    void on_device_description_path_selector_clicked();
    void on_rpc_xml_files_path_selector_clicked();
    void on_favorite_script_selector_clicked();
    void on_lua_editor_path_selector_clicked();
    void on_meta_path_selector_clicked();
    void on_env_var_path_button_clicked();
    void on_exceptional_approval_path_clicked();
    void on_isotope_db_path_button_clicked();
    void on_search_path_textChanged(const QString &arg1);
    void on_btn_cancel_clicked();
    void on_btn_ok_clicked();
    void on_btn_export_clicked();
    void on_btn_import_clicked();
    void on_confirm_keySequenceEdit_clear_button_clicked();
    void on_skip_keySequenceEdit_clear_button_clicked();
    void on_cancel_keySequenceEdit_clear_button_clicked();
    void on_report_history_query_selector_clicked();
    void on_luacheck_path_button_clicked();

    void on_edit_libpaths_textChanged(const QString &arg1);

    private:
    Ui::SettingsForm *ui;
    std::vector<std::pair<QLineEdit *, const char *>> get_config_lines() const;
    void write_ui_to_settings(QSettings &q_settings);
    void load_ui_from_settings(QSettings &q_settings);

    struct Key_config {
        QKeySequenceEdit *edit_field{};
        const char *config{};
        const char *default_key{};
    };

    std::vector<SettingsForm::Key_config> get_key_sequence_config() const;
};

#endif // SETTINGSFORM_H
