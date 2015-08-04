#include "communicationdevice.h"
#include "socketcommunicationdevice.h"
#include "echocommunicationdevice.h"
#include <regex>

CommunicationDevice *CommunicationDevice::createConnection(QString target){
	std::regex ipPort(R"(([[:digit:]]{1,3}\.){3}[[:digit:]]{1,3}:[[:digit:]]{1,5})");
	if (target == "echo"){
		return new EchoCommunicationDevice();
	}
	if (regex_match(target.toStdString(), ipPort)){
		return new SocketCommunicationDevice();
	}
	return nullptr;
}
