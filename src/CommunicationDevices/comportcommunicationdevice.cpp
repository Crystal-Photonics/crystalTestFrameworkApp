#include "comportcommunicationdevice.h"

#include <QApplication>
#include <cassert>

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

bool ComportCommunicationDevice::waitReceived(Duration timeout, int bytes) {
	auto now = std::chrono::high_resolution_clock::now();
	int received_bytes = 0;
	do {
		QApplication::processEvents();
		auto result = port.readAll();
		if (result.isEmpty() == false) {
			emit received(result);
			received_bytes += result.size();
		}
	} while (received_bytes < bytes && std::chrono::high_resolution_clock::now() - now < timeout);
	return received_bytes >= bytes;
}

void ComportCommunicationDevice::send(const QByteArray &data) {
	//TODO: somehow handle not being able to write data
	auto size = port.write(data);
	if (size == -1) {
		return;
	}
	if (size != data.size()) {
		size += port.write(data.data() + size, data.size() - size);
		assert(size == data.size());
	}
	emit sent(data);
}

void ComportCommunicationDevice::close() {
	port.close();
}
