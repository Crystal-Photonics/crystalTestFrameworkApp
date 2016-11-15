#include "socketcommunicationdevice.h"
#include <regex>
#include <cassert>
#include <exception>
#include "util.h"
#include <memory>
#include <QDebug>

SocketCommunicationDevice::SocketCommunicationDevice()
	: socket(nullptr)
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
		isServer = false;
		socket = new QTcpSocket;
		socket->connectToHost(ip, port);
		receiveSlot = connect(socket, QTcpSocket::readyRead, [this]()
			{
				emit this->receiveData(this->socket->readAll());
			}
		);
		assert(receiveSlot);
	}
	else if (type == "server"){
		isServer = true;
		auto success = server.listen(QHostAddress(ip), port);
		if (!success)
			throw std::runtime_error("Failed opening " + ip.toStdString() + ':' + std::to_string(port));
		callSetSocket = [this](){
			this->setSocket();
			this->connected();
		};
		connectedSlot = QObject::connect(&server, QTcpServer::newConnection, callSetSocket);
		assert(connectedSlot);
	}
	else{
		throw std::logic_error("regex logic is wrong: " + type.toStdString() + " must be \"server\" or \"client\"");
	}
}

void SocketCommunicationDevice::send(const QByteArray &data)
{
	socket->write(data);
	socket->waitForBytesWritten(1000);
}

SocketCommunicationDevice::~SocketCommunicationDevice()
{
	QObject::disconnect(connectedSlot);
	QObject::disconnect(receiveSlot);
}

bool SocketCommunicationDevice::waitConnected(std::chrono::seconds timeout)
{
	if (isServer){
		return server.waitForNewConnection(timeout.count());
	}
	return socket->waitForConnected(timeout.count());
}

bool SocketCommunicationDevice::waitReceived(std::chrono::seconds timeout)
{
	return socket->waitForReadyRead(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
}

void SocketCommunicationDevice::setSocket()
{
	socket = server.nextPendingConnection();
	if (receiveSlot)
		QObject::disconnect(receiveSlot);
	receiveSlot = connect(socket, QTcpSocket::readyRead, [this]()
		{
			emit this->receiveData(this->socket->readAll());
		}
	);
	assert(receiveSlot);
}

bool SocketCommunicationDevice::isConnected()
{
	if (!socket)
		return false;
	return socket->state() == QAbstractSocket::ConnectedState;
}

void SocketCommunicationDevice::receiveData(QByteArray data)
{
	emit received(std::move(data));
}