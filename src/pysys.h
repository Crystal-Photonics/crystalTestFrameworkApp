#ifndef PYSYS_H
#define PYSYS_H

#include <QObject>
#include <QVariant>

class pySys : public QObject
{
    Q_OBJECT
public:
    explicit pySys(QObject *parent = 0);
    ~pySys();

public Q_SLOTS:
    virtual void out(QVariant text);
};


#endif // PYSYS_H
