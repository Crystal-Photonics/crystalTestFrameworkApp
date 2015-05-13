#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QString>

class ScriptEngine : public QObject
{
    Q_OBJECT
public:
    explicit ScriptEngine( QString dir, QObject *parent = 0);
    ~ScriptEngine();


signals:

public slots:

private:
    QString dir;
};

#endif // SCRIPTENGINE_H
