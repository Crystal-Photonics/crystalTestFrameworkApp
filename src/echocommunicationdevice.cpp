#include "echocommunicationdevice.h"
#include <memory>

void EchoCommunicationDevice::send(const QByteArray &data)
{
	emit receive(std::move(data));
}
