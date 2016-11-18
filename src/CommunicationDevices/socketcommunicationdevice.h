#ifndef SOCKETCOMMUNICATIONDEVICE_H
#define SOCKETCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include "export.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <functional>
#include <memory>

class EXPORT SocketCommunicationDevice final : public CommunicationDevice {
	public:
	SocketCommunicationDevice();
	void send(const QByteArray &data) override;
	~SocketCommunicationDevice();
	bool awaitConnection(Duration timeout = std::chrono::seconds(1));
	bool waitReceived(Duration timeout = std::chrono::seconds(1), int bytes = 1) override;
	void close() override;

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
