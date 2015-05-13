#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QString>
#include <QList>

class ScriptEngine : public QObject
{
    Q_OBJECT
public:
    explicit ScriptEngine( QString dir, QObject *parent = 0);
    ~ScriptEngine();

    QList<QString> getFilesInDirectory();


signals:

public slots:

private:
    QString scriptDir;
};

#endif // SCRIPTENGINE_H
