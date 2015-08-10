#ifndef SOCKETCOMMUNICATIONDEVICE_H
#define SOCKETCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include "export.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <memory>

class EXPORT SocketCommunicationDevice : public CommunicationDevice
{
public:
	SocketCommunicationDevice(const QString &target);
	void send(const QByteArray &data) override;
private:
	std::unique_ptr<QTcpSocket> socket; //QTcpSocket does not support move semantics
	QTcpServer server;
	void connected();
};

#endif // SOCKETCOMMUNICATIONDEVICE_H
