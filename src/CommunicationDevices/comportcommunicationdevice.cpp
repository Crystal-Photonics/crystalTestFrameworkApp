#include "comportcommunicationdevice.h"


bool ComportCommunicationDevice::isConnected()
{
	return false;
}

bool ComportCommunicationDevice::waitConnected(std::chrono::seconds timeout)
{
	(void)timeout;
	return false;
}

bool ComportCommunicationDevice::waitReceived(std::chrono::seconds timeout)
{
	(void)timeout;
	return false;
}

void ComportCommunicationDevice::send(const QByteArray &data)
{
	(void)data;
	return;
}