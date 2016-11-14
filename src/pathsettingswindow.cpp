#include "pathsettingswindow.h"
#include "ui_pathsettingswindow.h"

#include <QSettings>
#include <QDebug>

namespace Globals{
	static const auto test_script_path_key = "test script path";
	static const auto detection_script_path_key = "detection script path";
}

PathSettingsWindow::PathSettingsWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PathSettingsWindow)
{
	ui->setupUi(this);
	ui->test_script_path_text->setText(QSettings{}.value(Globals::test_script_path_key, "").toString());
	ui->detection_script_path_text->setText(QSettings{}.value(Globals::detection_script_path_key, "").toString());
	qDebug() << "getting" << QSettings{}.value(Globals::detection_script_path_key, "").toString();
}

PathSettingsWindow::~PathSettingsWindow()
{
	delete ui;
}

void PathSettingsWindow::on_settings_confirmation_accepted()
{
	qDebug() << "setting" << ui->detection_script_path_text->text();
	QSettings settings;
	settings.setValue(Globals::detection_script_path_key, ui->detection_script_path_text->text());
	settings.setValue(Globals::test_script_path_key, ui->test_script_path_text->text());
	settings.sync();
	qDebug() << "getting" << settings.value(Globals::detection_script_path_key, "").toString();
}
