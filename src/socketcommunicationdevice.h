#ifndef SOCKETCOMMUNICATIONDEVICE_H
#define SOCKETCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"

class SocketCommunicationDevice : public CommunicationDevice
{
public:
	SocketCommunicationDevice();
	~SocketCommunicationDevice();
};

#endif // SOCKETCOMMUNICATIONDEVICE_H
