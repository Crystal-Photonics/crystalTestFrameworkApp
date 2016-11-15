#ifndef COMMUNICATIONDEVICE_H
#define COMMUNICATIONDEVICE_H

#include "export.h"
#include <QByteArray>
#include <QObject>
#include <chrono>
#include <memory>

class EXPORT CommunicationDevice : public QObject {
    Q_OBJECT
	protected:
	CommunicationDevice() = default;

    public:
	virtual ~CommunicationDevice() = default;
	static std::unique_ptr<CommunicationDevice> createConnection(const QString &target);
	virtual bool isConnected() = 0;
	virtual bool waitConnected(std::chrono::seconds timeout = std::chrono::seconds(1)) = 0;
	virtual bool waitReceived(std::chrono::seconds timeout = std::chrono::seconds(1)) = 0;
	virtual void send(const QByteArray &data) = 0;
	bool operator ==(const QString &target) const;
	const QString &getTarget() const;

    signals:
	void connected();
	void disconnected();
	void received(QByteArray);
	public slots:
protected:
    QString target;
};

#endif // COMMUNICATIONDEVICE_H
