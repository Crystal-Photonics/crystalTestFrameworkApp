#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QFile>

#include "pysys.h"
#include "export.h"

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
    void slotPythonStdOut(const QString &str);
    void slotPythonStdErr(const QString &str);
private:
    QString scriptDir;
    pySys *rtSys;
    QFile fileStdOut;
};

#endif // SCRIPTENGINE_H
