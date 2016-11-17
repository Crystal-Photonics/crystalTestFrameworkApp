#ifndef COMPORTCOMMUNICATIONDEVICE_H
#define COMPORTCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"

class ComportCommunicationDevice : public CommunicationDevice {
	public:
	ComportCommunicationDevice(QString target);
	bool isConnected() override;
	bool waitConnected(Duration timeout = std::chrono::seconds(1), const QString &params = "") override;
	bool waitReceived(Duration timeout = std::chrono::seconds(1)) override;
	void send(const QByteArray &data) override;
};

#endif // COMPORTCOMMUNICATIONDEVICE_H
