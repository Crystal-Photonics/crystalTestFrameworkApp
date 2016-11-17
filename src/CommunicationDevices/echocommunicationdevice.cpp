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

bool EchoCommunicationDevice::waitConnected(Duration timeout, const QString &params) {
	(void)timeout;
	(void)params;
	return true;
}

bool EchoCommunicationDevice::waitReceived(Duration timeout) {
	//TODO: actually wait
	(void)timeout;
	return true;
}
