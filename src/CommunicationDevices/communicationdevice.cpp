#include "communicationdevice.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "echocommunicationdevice.h"
#include "socketcommunicationdevice.h"

#include <QDebug>
#include <regex>

std::unique_ptr<CommunicationDevice> CommunicationDevice::createConnection(const QString &target) {
	const auto targetstring = target.toStdString();
	if (targetstring == "echo") {
		return std::make_unique<EchoCommunicationDevice>();
	}
	std::regex ipPort(R"(((server:)|(client:))([[:digit:]]{1,3}\.){3}[[:digit:]]{1,3}:[[:digit:]]{1,5})");
	if (regex_match(targetstring, ipPort)) {
		return std::make_unique<SocketCommunicationDevice>();
	}
	std::regex comport(R"(\\\\.\\COM[[:digit:]]+)");
	if (regex_match(targetstring, comport)) {
		return std::make_unique<ComportCommunicationDevice>(target);
	}
	qDebug() << "unknown target device" << target;
	return nullptr;
}

void CommunicationDevice::send(const std::vector<unsigned char> &data, const std::vector<unsigned char> &displayed_data) {
    size_t datasize = data.size();
    send(QByteArray(reinterpret_cast<const char *>(data.data()), datasize),
		 QByteArray(reinterpret_cast<const char *>(displayed_data.data()), displayed_data.size()));
}

bool CommunicationDevice::operator==(const QString &target) const {
	return this->target == target;
}

const QString &CommunicationDevice::getTarget() const {
    return target;
}

bool CommunicationDevice::is_waiting_for_message() const
{
    return currently_in_waitReceived;
}

void CommunicationDevice::set_currently_in_wait_received(bool in_wait_received)
{
    currently_in_waitReceived = in_wait_received;
}





