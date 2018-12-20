#ifndef COMPORTCOMMUNICATIONDEVICE_H
#define COMPORTCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>


class ComportCommunicationDevice : public CommunicationDevice {
	public:
    ComportCommunicationDevice();
	bool isConnected() override;
    bool connect(const QMap<QString,QVariant> &portinfo_) override;
	bool waitReceived(Duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling = false) override;
    bool waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line) override;
	void send(const QByteArray &data, const QByteArray &displayed_data = {}) override;
	void close() override;
    QSerialPort port;
    QString getName() override;
};

#endif // COMPORTCOMMUNICATIONDEVICE_H
