#include "echocommunicationdevice.h"
#include <memory>

EchoCommunicationDevice::EchoCommunicationDevice()
{
	emit connected();
}

void EchoCommunicationDevice::send(const QByteArray &data)
{
	emit receive(std::move(data));
}

bool EchoCommunicationDevice::isConnected()
{
	return true;
}

bool EchoCommunicationDevice::waitConnected(std::chrono::seconds timeout)
{
	(void)timeout;
	return true;
}

bool EchoCommunicationDevice::waitReceive(std::chrono::seconds timeout)
{
	//TODO: actually wait
	(void)timeout;
	return true;
}
