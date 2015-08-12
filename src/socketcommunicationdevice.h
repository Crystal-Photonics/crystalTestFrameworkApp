#ifndef SOCKETCOMMUNICATIONDEVICE_H
#define SOCKETCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include "export.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <memory>
#include <functional>

class EXPORT SocketCommunicationDevice : public CommunicationDevice
{
public:
	SocketCommunicationDevice(const QString &target);
	void send(const QByteArray &data) override;
	~SocketCommunicationDevice();
	bool waitConnected(std::chrono::seconds timeout = std::chrono::seconds(1)) override;
	bool waitReceive(std::chrono::seconds timeout = std::chrono::seconds(1)) override;
private:
	QTcpSocket *socket; //QTcpSocket does not support move semantics and is somehow auto-deleted, probably
	QTcpServer server;
	void setSocket();
	QMetaObject::Connection connectedSlot, receiveSlot;
	std::function<void()> callSetSocket;
	bool isConnected() override;
	void receiveData(QByteArray data);
	bool isServer;
};

#endif // SOCKETCOMMUNICATIONDEVICE_H
