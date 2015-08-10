#include "socketcommunicationdevice.h"
#include <regex>
#include <cassert>
#include <exception>
#include "util.h"
#include <memory>
#include <QDebug>

SocketCommunicationDevice::SocketCommunicationDevice(const QString &target)
{
	std::regex ipPort(R"(((server:)|(client:))([[:digit:]]{1,3}\.){3}[[:digit:]]{1,3}:[[:digit:]]{1,5})");
	auto success = std::regex_match(target.toStdString(), ipPort);
	assert(success);
	auto typeIpPort = target.split(':');
	auto &type = typeIpPort[0];
	auto &ip = typeIpPort[1];
	int port;
	success = Utility::convert(typeIpPort[2], port);
	assert(success);
	server.setMaxPendingConnections(1);
	if (type == "client"){
		socket = std::make_unique<QTcpSocket>();
		socket->connectToHost(ip, port);
	}
	else if (type == "server"){
		auto success = server.listen(QHostAddress(ip), port);
		if (!success)
			throw std::runtime_error("Failed opening " + ip.toStdString() + ':' + std::to_string(port));
	}
	else{
		throw std::logic_error("regex logic is wrong: " + type.toStdString() + " must be \"server\" or \"client\"");
	}
}

void SocketCommunicationDevice::send(const QByteArray &data)
{
	socket->write(data);
}

void SocketCommunicationDevice::connected()
{
	socket.reset(server.nextPendingConnection());
}
