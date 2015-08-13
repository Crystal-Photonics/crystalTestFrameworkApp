#ifndef ECHOCOMMUNICATIONDEVICE_H
#define ECHOCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include <QObject>
#include "export.h"

class EXPORT EchoCommunicationDevice final : public CommunicationDevice
{
	Q_OBJECT
public:
	EchoCommunicationDevice();
	void send(const QByteArray &data) override;
	bool isConnected() override;
	bool waitConnected(std::chrono::seconds timeout = std::chrono::seconds(1)) override;
	bool waitReceive(std::chrono::seconds timeout = std::chrono::seconds(1)) override;
};

#endif // ECHOCOMMUNICATIONDEVICE_H
