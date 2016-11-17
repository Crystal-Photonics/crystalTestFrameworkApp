#include "comportcommunicationdevice.h"

ComportCommunicationDevice::ComportCommunicationDevice(QString target) {
	this->target = target;
}

bool ComportCommunicationDevice::isConnected() {
	return false;
}

bool ComportCommunicationDevice::waitConnected(Duration timeout, const QString &params) {
	(void)timeout;
	(void)params;
	return false;
}

bool ComportCommunicationDevice::waitReceived(Duration timeout) {
	(void)timeout;
	return false;
}

void ComportCommunicationDevice::send(const QByteArray &data) {
	(void)data;
	return;
}
