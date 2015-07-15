#include <QDebug>
#include <QDir>
#include "scriptengine.h"
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

void ScriptEngine::slotPythonStdOut (const QString &str){

    qDebug() << str;
}

void ScriptEngine::slotPythonStdErr (const QString &str){

    qWarning() << str;
}



ScriptEngine::ScriptEngine(QString dir, QObject *parent) : QObject(parent)
{
    this->scriptDir = dir;
    rtSys = new pySys(this);
}

ScriptEngine::~ScriptEngine()
{


}

void ScriptEngine::setRTSys(pySys *rtSys_)
{
    this->rtSys = rtSys_;
}

QList<QString> ScriptEngine::getFilesInDirectory(){
    QList<QString> result;
    QDir scriptDirectory(scriptDir);
    QStringList filter;
    filter << "*.py";
    scriptDirectory.setNameFilters(filter);
    foreach (QString fileName, scriptDirectory.entryList(QDir::Files)) {
        //qDebug() << fileName;
        result.append(fileName);
    }
    return result;
}
/*
int ScriptEngine::scriptPrint(QString fileName){

}
*/
void ScriptEngine::runScript(QString fileName){
    PythonQt::init(PythonQt::IgnoreSiteModule | PythonQt::RedirectStdOut );
    QDir dir(scriptDir);
    fileName = dir.absoluteFilePath(fileName);

    PythonQtObjectPtr mainModule = PythonQt::self()->getMainModule();

    QObject::connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString)),
                         this, SLOT(slotPythonStdOut(const QString)));
    QObject::connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString)),
                         this, SLOT(slotPythonStdErr(const QString)));

    if (fileExists(fileName)){
        mainModule.addObject("sys", rtSys);
        QFile t(fileName);
        t.open(QIODevice::ReadOnly| QIODevice::Text);
        QTextStream out(&t);
        mainModule.evalScript(out.readAll());
        t.close();

    }
}

#if 0
bool ScriptEngine::pythonTest(){

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
#endif
