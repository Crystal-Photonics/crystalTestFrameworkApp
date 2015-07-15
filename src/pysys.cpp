#include <QDebug>
#include "pysys.h"

pySys::pySys(QObject *parent): QObject(parent)
{
    //qDebug() << "pySys constructed";
}

void pySys::out(QVariant text){
    qDebug() << text.toString();
}

pySys::~pySys()
{
    //qDebug() << "pySys destructed";
}

