#ifndef COMMUNICATIONDEVICE_H
#define COMMUNICATIONDEVICE_H

#include <QObject>
#include <QByteArray>
#include <memory>

class CommunicationDevice : public QObject
{
	Q_OBJECT
public:
	explicit CommunicationDevice(QObject *parent = nullptr);
	virtual ~CommunicationDevice();
	static CommunicationDevice *connect(QString target, QObject *parent = nullptr);
signals:
	void connected();
	void disconnected();
	void receive(QByteArray data);
public slots:
	virtual void send(const QByteArray &data) = 0;
};

#endif // COMMUNICATIONDEVICE_H
