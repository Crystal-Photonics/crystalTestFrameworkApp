#include "mainwindow.h"
#include "config.h"
#include "pathsettingswindow.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_actionPaths_triggered() {
	path_dialog = new PathSettingsWindow(this);
	path_dialog->show();
}

void MainWindow::on_device_detect_button_clicked() {
	const auto script_dir = QSettings{}.value(Globals::detection_script_path_settings_key, "").toString();
	QDirIterator it(script_dir, QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		//ScriptEngine{}.run_detection_script(it.next());
	}
}
