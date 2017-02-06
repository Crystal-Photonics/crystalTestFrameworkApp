#include "comportcommunicationdevice.h"
#include "qt_util.h"

#include <QApplication>
#include <cassert>

ComportCommunicationDevice::ComportCommunicationDevice(QString target) {
	this->target = target;
}

bool ComportCommunicationDevice::isConnected() {
	return Utility::promised_thread_call(this, [this] { return port.isOpen(); });
}

bool ComportCommunicationDevice::connect(const QSerialPortInfo &portinfo, QSerialPort::BaudRate baudrate) {
	return Utility::promised_thread_call(this, [this, portinfo, baudrate] {
		port.setPort(portinfo);
		port.setBaudRate(baudrate);
		return port.open(QIODevice::ReadWrite);
	});
}

bool ComportCommunicationDevice::waitReceived(Duration timeout, int bytes) {
	return Utility::promised_thread_call(this, [this, timeout, bytes] {
		auto now = std::chrono::high_resolution_clock::now();
		int received_bytes = 0;
		auto try_read = [this, &received_bytes] {
			QApplication::processEvents();
			auto result = port.readAll();
			if (result.isEmpty() == false) {
				emit received(result);
				received_bytes += result.size();
			}
		};
		currently_in_waitReceived = true;
		do {
			try_read();
		} while (received_bytes < bytes && std::chrono::high_resolution_clock::now() - now < timeout);
		try_read();
		QApplication::processEvents();
		currently_in_waitReceived = false;
		return received_bytes >= bytes;
	});
}

void ComportCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
	return Utility::promised_thread_call(this, [this, data, displayed_data] {
		auto size = port.write(data);
		if (size == -1) {
			return;
		}
		if (size != data.size()) {
			size += port.write(data.data() + size, data.size() - size);
			assert(size == data.size());
		}
		emit decoded_sent(displayed_data.isEmpty() ? data : displayed_data);
		emit sent(data);
	});
}

void ComportCommunicationDevice::close() {
	return Utility::promised_thread_call(this, [this] { port.close(); });
}
