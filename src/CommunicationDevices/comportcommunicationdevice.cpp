#include "comportcommunicationdevice.h"

ComportCommunicationDevice::ComportCommunicationDevice(QString target) {
	this->target = target;
}

bool ComportCommunicationDevice::isConnected() {
	return port.isOpen();
}

bool ComportCommunicationDevice::connect(const QSerialPortInfo &portinfo, QSerialPort::BaudRate baudrate) {
	port.setPort(portinfo);
	port.setBaudRate(baudrate);
	return port.open(QIODevice::ReadWrite);
}

bool ComportCommunicationDevice::waitReceived(Duration timeout) {
	(void)timeout;
	return false;
}

void ComportCommunicationDevice::send(const QByteArray &data) {
	(void)data;
	return;
}

void ComportCommunicationDevice::close()
{
	port.close();
}
