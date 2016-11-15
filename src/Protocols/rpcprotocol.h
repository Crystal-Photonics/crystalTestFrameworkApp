#ifndef RPCPROTOCOL_H
#define RPCPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"

class RPCProtocol
{
public:
	static bool is_correct_protocol(CommunicationDevice *device);
};

#endif // RPCPROTOCOL_H
