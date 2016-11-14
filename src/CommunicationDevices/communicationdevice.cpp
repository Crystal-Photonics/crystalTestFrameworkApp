#include "communicationdevice.h"
#include "socketcommunicationdevice.h"
#include "echocommunicationdevice.h"

#include <regex>

std::unique_ptr<CommunicationDevice> CommunicationDevice::createConnection(QString target){
	std::regex ipPort(R"(((server:)|(client:))([[:digit:]]{1,3}\.){3}[[:digit:]]{1,3}:[[:digit:]]{1,5})");
	if (target == "echo"){
		return std::make_unique<EchoCommunicationDevice>();
	}
	if (regex_match(target.toStdString(), ipPort)){
		return std::make_unique<SocketCommunicationDevice>(target);
	}
	//TODO: comports
	return nullptr;
}
