#ifndef COMMUNICATIONDEVICE_H
#define COMMUNICATIONDEVICE_H

#include <QObject>
#include <QByteArray>
#include <memory>
#include "export.h"
#include <chrono>

class EXPORT CommunicationDevice : public QObject
{
	Q_OBJECT
protected:
	CommunicationDevice() = default;
public:
	virtual ~CommunicationDevice() = default;
	static std::unique_ptr<CommunicationDevice> createConnection(QString target);
	virtual bool isConnected() = 0;
	virtual bool waitConnected(std::chrono::seconds timeout = std::chrono::seconds(1)) = 0;
	virtual bool waitReceive(std::chrono::seconds timeout = std::chrono::seconds(1)) = 0;
signals:
	void connected();
	void disconnected();
	void receive(QByteArray);
public slots:
	virtual void send(const QByteArray &data) = 0;
};

#endif // COMMUNICATIONDEVICE_H
