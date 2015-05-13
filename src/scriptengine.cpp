#include <QDebug>
#include <QDir>
#include "scriptengine.h"

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
