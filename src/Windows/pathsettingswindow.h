#ifndef PATHSETTINGSWINDOW_H
#define PATHSETTINGSWINDOW_H

#include <QDialog>
#include <vector>

class QLineEdit;

namespace Ui {
	class PathSettingsWindow;
}

class PathSettingsWindow : public QDialog {
	Q_OBJECT

	public:
	explicit PathSettingsWindow(QWidget *parent = 0);
	~PathSettingsWindow();

	private slots:
	void on_settings_confirmation_accepted();
	void on_settings_confirmation_rejected();
	void on_test_script_path_selector_clicked();
	void on_device_description_path_selector_clicked();
	void on_rpc_xml_files_path_selector_clicked();
	void on_lua_editor_path_selector_clicked();
	void on_meta_path_selector_clicked();
    void on_env_var_path_button_clicked();
    void on_exceptional_approval_path_clicked();
	void on_isotope_db_path_button_clicked();

    void on_search_path_textChanged(const QString &arg1);

private:
	Ui::PathSettingsWindow *ui;

	std::vector<std::pair<QLineEdit *, const char *>> get_config_lines() const;
};

#endif // PATHSETTINGSWINDOW_H
