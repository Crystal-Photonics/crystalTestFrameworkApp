#include "mainwindow.h"
#include "scriptengine.h"
#include "ui_mainwindow.h"

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
