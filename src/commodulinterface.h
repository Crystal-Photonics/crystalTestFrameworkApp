#ifndef COMMODULINTERFACE_H
#define COMMODULINTERFACE_H

#include <QString>

//! [0]
class comModulInterface
{
public:
    virtual ~comModulInterface() {}
    virtual QString echo(const QString &message) = 0;
};


QT_BEGIN_NAMESPACE

#define comModulInterface_iid "de.crystal-photonics.crystalTestFramework.comModulInterface"

Q_DECLARE_INTERFACE(comModulInterface, comModulInterface_iid)
QT_END_NAMESPACE


#endif // COMMODULINTERFACE_H

