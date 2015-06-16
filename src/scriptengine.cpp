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

ScriptEngine::ScriptEngine(QString dir, QObject *parent) : QObject(parent)
{
    this->scriptDir = dir;
}

ScriptEngine::~ScriptEngine()
{

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
int ScriptEngine::runScript(QString fileName){
    PythonQt::init();

    // get the __main__ python module
    PythonQtObjectPtr mainModule = PythonQt::self()->getMainModule();
    QVariant result = mainModule.evalScript("19*2+4", Py_eval_input);
    qDebug() << "result = " << result.toInt();
    if (fileExists(fileName)){

        mainModule.evalFile(fileName);

       // QVariant result =
        // mainModule.evalScript("def appendText(text):\n  box.browser.append(text)");
         // shows how to call the method with a text that will be append to the browser
        //mainModule.call("appendText", QVariantList() << "The ultimate answer is ");
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
