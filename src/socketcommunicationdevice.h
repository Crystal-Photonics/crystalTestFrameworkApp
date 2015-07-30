#ifndef SOCKETCOMMUNICATIONDEVICE_H
#define SOCKETCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"

class SocketCommunicationDevice : public CommunicationDevice
{
public:
	void send(const QByteArray &data) override;
};

#endif // SOCKETCOMMUNICATIONDEVICE_H
