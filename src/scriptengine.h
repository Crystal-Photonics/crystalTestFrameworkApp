#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QFile>

#include "export.h"

class EXPORT ScriptEngine : public QObject
{
    Q_OBJECT
public:
	explicit ScriptEngine(QObject *parent = nullptr);
    ~ScriptEngine();
};

#endif // SCRIPTENGINE_H
