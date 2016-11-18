#include "echocommunicationdevice.h"
#include <memory>

EchoCommunicationDevice::EchoCommunicationDevice() {
	emit connected();
}

void EchoCommunicationDevice::send(const QByteArray &data) {
	emit received(std::move(data));
}

bool EchoCommunicationDevice::isConnected() {
	return true;
}

bool EchoCommunicationDevice::waitReceived(Duration timeout, int bytes) {
	//TODO: actually wait
	(void)timeout;
	(void)bytes;
	return true;
}

void EchoCommunicationDevice::close() {}
