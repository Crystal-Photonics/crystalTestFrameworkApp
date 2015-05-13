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
#include "PythonQt.h"

bool fileExists(QString path) {
    QFileInfo checkFile(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (checkFile.exists() && checkFile.isFile()) {
        return true;
    } else {
        return false;
    }
}

bool MainWindow::pythonTest(){

     PythonQt::init();

     // get the __main__ python module
     PythonQtObjectPtr mainModule = PythonQt::self()->getMainModule();

     // evaluate a simple python script and receive the result a qvariant:
     QVariant result = mainModule.evalScript("19*2+4", Py_eval_input);

     // create a small Qt GUI
     QVBoxLayout*  vbox = new QVBoxLayout;
     QGroupBox*    box  = new QGroupBox;
     QTextBrowser* browser = new QTextBrowser(box);
     QLineEdit*    edit = new QLineEdit(box);
     QPushButton*  button = new QPushButton(box);
     button->setObjectName("button1");
     edit->setObjectName("edit");
     browser->setObjectName("browser");
     vbox->addWidget(browser);
     vbox->addWidget(edit);
     vbox->addWidget(button);
     box->setLayout(vbox);

     // make the groupbox to the python under the name "box"
     mainModule.addObject("box", box);

     // evaluate the python script which is defined in the resources
     mainModule.evalFile("GettingStarted.py");
     if (fileExists("GettingStarted.py")){

     }else{
        QDir pluginsDir(qApp->applicationDirPath());
        qDebug() << "file doenst exist";
        qDebug() << pluginsDir.absolutePath();
     }
     #if 0// init PythonQt and Python
     // define a python method that appends the passed text to the browser
     mainModule.evalScript("def appendText(text):\n  box.browser.append(text)");
     // shows how to call the method with a text that will be append to the browser
     mainModule.call("appendText", QVariantList() << "The ultimate answer is ");
#endif
     return true;
}

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
    pythonTest();
}
