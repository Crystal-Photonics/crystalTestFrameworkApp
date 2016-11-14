#ifndef PATHSETTINGSWINDOW_H
#define PATHSETTINGSWINDOW_H

#include <QDialog>

namespace Ui {
	class PathSettingsWindow;
}

class PathSettingsWindow : public QDialog
{
	Q_OBJECT

public:
	explicit PathSettingsWindow(QWidget *parent = 0);
	~PathSettingsWindow();

private slots:
	void on_settings_confirmation_accepted();

private:
	Ui::PathSettingsWindow *ui;
};

#endif // PATHSETTINGSWINDOW_H
