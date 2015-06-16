#ifndef PYSYS_H
#define PYSYS_H

#include <QObject>
#include <QVariant>

class pySys : public QObject
{
    Q_OBJECT
public:
    explicit pySys(QObject *parent = 0);
    void print(QVariant text);
    ~pySys();

signals:

public slots:
};

#endif // PYSYS_H
