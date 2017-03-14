#include "echocommunicationdevice.h"
#include <memory>

EchoCommunicationDevice::EchoCommunicationDevice() {
	emit connected();
}

void EchoCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
	(void)displayed_data;
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

bool EchoCommunicationDevice::waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line){
    //TODO: actually wait
    (void)timeout;
    (void)escape_characters;
    return true;
}

void EchoCommunicationDevice::close() {}
