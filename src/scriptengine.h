#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QFile>

#include "pysys.h"
#include "export.h"
#include "PythonQt.h"


class EXPORT ScriptEngine : public QObject
{
    Q_OBJECT
public:
    explicit ScriptEngine(QString dir, QObject *parent = 0);
    ~ScriptEngine();

    void setRTSys(pySys *rtSys_);

    QList<QString> getFilesInDirectory();

    void runScript(QString fileName);



private slots:
    virtual void slotPythonStdOut(const QString &str);
    virtual void slotPythonStdErr(const QString &str);
private:
    QString scriptDir;
    pySys *rtSys;
    QFile fileStdOut;
    QFile fileStdErr;
    QByteArray scriptArgv;
    PythonQtObjectPtr mainModule;
	const char *pScriptArgv;
};

#endif // SCRIPTENGINE_H
