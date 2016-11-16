#ifndef COMPORTCOMMUNICATIONDEVICE_H
#define COMPORTCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"

class ComportCommunicationDevice : public CommunicationDevice
{
public:
	ComportCommunicationDevice(QString target);
	bool isConnected() override;
	bool waitConnected(std::chrono::seconds timeout = std::chrono::seconds(1)) override;
	bool waitReceived(std::chrono::seconds timeout = std::chrono::seconds(1)) override;
	void send(const QByteArray &data) override;
};

#endif // COMPORTCOMMUNICATIONDEVICE_H
