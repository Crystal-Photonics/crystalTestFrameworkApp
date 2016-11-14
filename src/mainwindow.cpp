#include "mainwindow.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"
#include "pathsettingswindow.h"

#include <QDir>
#include <QPluginLoader>
#include <QDebug>

#include <QTextBrowser>
#include <QLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionPaths_triggered()
{
	path_dialog = new PathSettingsWindow(this);
	path_dialog->show();
}
