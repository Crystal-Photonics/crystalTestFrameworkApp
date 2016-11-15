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
		return std::make_unique<ComportCommunicationDevice>();
	}
	qDebug() << "unknown target device" << target;
	return nullptr;
}

bool CommunicationDevice::operator==(const QString &target) const {
	return this->target == target;
}

const QString &CommunicationDevice::getTarget() const {
	return target;
}
