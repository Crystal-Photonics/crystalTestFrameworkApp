#ifndef ECHOCOMMUNICATIONDEVICE_H
#define ECHOCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include "export.h"
#include <QObject>

class EXPORT EchoCommunicationDevice final : public CommunicationDevice {
	Q_OBJECT
	public:
	EchoCommunicationDevice();
    bool connect(const QMap<QString,QVariant> &portinfo_) override;
	void send(const QByteArray &data, const QByteArray &displayed_data = {}) override;
	bool isConnected() override;
    bool waitReceived(Duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling = false) override;
    bool waitReceived(Duration timeout, std::string escape_characters, std::string sleading_pattern_indicating_skip_line) override;
	void close() override;
	QString getName() override;
};

#endif // ECHOCOMMUNICATIONDEVICE_H
