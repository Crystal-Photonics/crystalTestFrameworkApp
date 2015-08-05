#include <QDebug>
#include <QDir>
#include "scriptengine.h"


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
    qDebug() << fileStdOut.write(ba);
   // qDebug() << str;
}

void ScriptEngine::slotPythonStdErr (const QString &str){
    QByteArray ba;
    ba.append(str);
    qDebug() << fileStdErr.write(ba);
    //qWarning() << str;
}

ScriptEngine::ScriptEngine(QString dir, QObject *parent) : QObject(parent)
{
    this->scriptDir = dir;
    rtSys = new pySys(this);
    PythonQt::init(PythonQt::IgnoreSiteModule | PythonQt::RedirectStdOut );
    mainModule = PythonQt::self()->getMainModule();

    QObject::connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString)),
                         this, SLOT(slotPythonStdOut(const QString)));
    QObject::connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString)),
                         this, SLOT(slotPythonStdErr(const QString)));

    //PythonQt::self()->addSysPath(info.absolutePath());
    PythonQt::self()->addSysPath("C:\\Python27\\lib");

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

    QDir dir(scriptDir);
    QString scriptFileName = dir.absoluteFilePath(fileName);





    QString stdOutName = fileName.split('.')[0]+"_stdout.txt";
    QString stdErrName = fileName.split('.')[0]+"_stderr.txt";

    //fileStdOut.close();
    fileStdOut.setFileName(dir.absoluteFilePath(stdOutName));
    if (!fileStdOut.open(QIODevice::Truncate | QIODevice::WriteOnly)){
        qWarning() << "Couldn't open stdout file";
    }

    //fileStdErr.close();
    fileStdErr.setFileName(dir.absoluteFilePath(stdErrName));
    if (!fileStdErr.open(QIODevice::Truncate | QIODevice::WriteOnly)){
        qWarning() << "Couldn't open stdout file";
    }
    if (fileExists(scriptFileName)){
#if 1
        scriptArgv = scriptFileName.toLatin1();
        pScriptArgv = "C:\\Users\\ark\\entwicklung\\qt\\crystalTestFrameworkApp\\builds\\crystalTestFrameworkApp-Desktop_Qt_5_4_1_MinGW_32bit-Debug\\tests\\scripts\\Test_UnitTest.py";
        //pScriptArgv = scriptArgv.data();
        PySys_SetArgv(1,&pScriptArgv);
#endif
        //QFileInfo info(scriptFileName);
        QFile f(scriptFileName);
        if (f.open(QIODevice::ReadOnly| QIODevice::Text)){
            QTextStream out(&f);
            // 'C:\\Python33', 'C:\\Python33\\python33.zip', 'C:\\Python33\\DLLs', 'C:\\Python33\\lib', 'C:\\Python33\\lib\\site-packages'


            mainModule.addObject("be", rtSys);
            mainModule.addVariable("__file__", scriptFileName);
            QString data = out.readAll();
            mainModule.evalScript(data);
            //mainModule.evalFile();
        }else{
            slotPythonStdErr("Couldn't open File \""+scriptFileName+"\"");
        }
        f.close();

    }else{
        slotPythonStdErr("Didn't find File \""+scriptFileName+"\"");
    }
    fileStdOut.close();
    fileStdErr.close();

#if 0
    Traceback (most recent call last):
    File "<string>", line 1, in <module>
  ImportError: No module named unittest
#endif
}

