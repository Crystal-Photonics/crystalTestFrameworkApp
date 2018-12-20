#include "echocommunicationdevice.h"
#include <memory>

EchoCommunicationDevice::EchoCommunicationDevice() {
    emit connected(QByteArray(""));
}

bool EchoCommunicationDevice::connect(const QMap<QString, QVariant> &portinfo_)
{
    this->portinfo = portinfo_;
    return true;
}

void EchoCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
	(void)displayed_data;
	emit received(std::move(data));
}

bool EchoCommunicationDevice::isConnected() {
	return true;
}

bool EchoCommunicationDevice::waitReceived(Duration timeout, int bytes, bool isPolling) {
	//TODO: actually wait
	(void)timeout;
	(void)bytes;
    (void)isPolling;
	return true;
}

bool EchoCommunicationDevice::waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line){
    //TODO: actually wait
    (void)timeout;
    (void)escape_characters;
    (void)leading_pattern_indicating_skip_line;
    return true;
}

void EchoCommunicationDevice::close() {}

QString EchoCommunicationDevice::getName()
{
    return "echo";
}
