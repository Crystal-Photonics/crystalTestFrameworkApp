#ifndef COMMUNICATIONDEVICE_H
#define COMMUNICATIONDEVICE_H

#include <QObject>
#include <QByteArray>
#include <memory>

class CommunicationDevice : public QObject
{
	Q_OBJECT
protected:
	CommunicationDevice() = default;
public:
	virtual ~CommunicationDevice() = default;
	static CommunicationDevice *createConnection(QString target);
signals:
	void connected();
	void disconnected();
	void receive(QByteArray);
public slots:
	virtual void send(const QByteArray &data) = 0;
};

#endif // COMMUNICATIONDEVICE_H
