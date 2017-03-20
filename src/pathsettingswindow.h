#ifndef PATHSETTINGSWINDOW_H
#define PATHSETTINGSWINDOW_H

#include <QDialog>

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

	private:
	Ui::PathSettingsWindow *ui;
};

#endif // PATHSETTINGSWINDOW_H
