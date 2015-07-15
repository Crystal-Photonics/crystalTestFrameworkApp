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
    QByteArray ba;
    ba.append(str);
    fileStdOut.write(ba);
   // qDebug() << str;
}

void ScriptEngine::slotPythonStdErr (const QString &str){

    qWarning() << str;
}

ScriptEngine::ScriptEngine(QString dir, QObject *parent) : QObject(parent)
{
    this->scriptDir = dir;
    rtSys = new pySys(this);
}

ScriptEngine::~ScriptEngine(){}

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
        result.append(fileName);
    }
    return result;
}

void ScriptEngine::runScript(QString fileName){
    PythonQt::init(PythonQt::IgnoreSiteModule | PythonQt::RedirectStdOut );
    QDir dir(scriptDir);
    QString scriptFileName = dir.absoluteFilePath(fileName);

    PythonQtObjectPtr mainModule = PythonQt::self()->getMainModule();

    QObject::connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString)),
                         this, SLOT(slotPythonStdOut(const QString)));
    QObject::connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString)),
                         this, SLOT(slotPythonStdErr(const QString)));

    if (fileExists(scriptFileName)){
        QString stdOutName = fileName.split('.')[0]+"_stdout.txt";
        fileStdOut.close();
        fileStdOut.setFileName(dir.absoluteFilePath(stdOutName));
        fileStdOut.open(QIODevice::Truncate | QIODevice::WriteOnly);

        mainModule.addObject("sys", rtSys);
        QFile f(scriptFileName);
        f.open(QIODevice::ReadOnly| QIODevice::Text);
        QTextStream out(&f);
        mainModule.evalScript(out.readAll());
        f.close();
        fileStdOut.close();
    }
}

