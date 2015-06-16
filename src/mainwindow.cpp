#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QPluginLoader>
#include <QDebug>

#include <QTextBrowser>
#include <QLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>





bool MainWindow::loadPlugin()
{
    QDir pluginsDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release"){
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("comModules");
    foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
        qDebug() << fileName;
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            ComModulInterface = qobject_cast<comModulInterface *>(plugin);
            if (ComModulInterface){
                qDebug() << "plugin loaded";
                ComModulInterface->echo_("test");
                return true;
            }
        }
    }

    return false;
}

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

void MainWindow::on_pushButton_clicked()
{
    loadPlugin();
}

void MainWindow::on_pushButton_2_clicked()
{

}
