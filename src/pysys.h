#ifndef PYSYS_H
#define PYSYS_H

#include <QObject>
#include <QVariant>
#include "export.h"

class EXPORT pySys : public QObject
{
    Q_OBJECT
public:
    explicit pySys(QObject *parent = 0);
    ~pySys();

public Q_SLOTS:
    virtual void out(QVariant text);
};


#endif // PYSYS_H
