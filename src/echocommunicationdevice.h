#ifndef ECHOCOMMUNICATIONDEVICE_H
#define ECHOCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include <QObject>
#include "export.h"

class EXPORT EchoCommunicationDevice : public CommunicationDevice
{
	Q_OBJECT
public:
	void send(const QByteArray &data) override;
};

#endif // ECHOCOMMUNICATIONDEVICE_H
