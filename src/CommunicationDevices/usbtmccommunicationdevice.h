#ifndef USBTMCCOMMUNICATIONDEVICE_H
#define USBTMCCOMMUNICATIONDEVICE_H
#include "CommunicationDevices/communicationdevice.h"
#include "CommunicationDevices/usbtmc.h"
class USBTMCCommunicationDevice : public CommunicationDevice {
    public:
    USBTMCCommunicationDevice();
    ~USBTMCCommunicationDevice();



    bool isConnected() override;
    bool connect(const QMap<QString, QVariant> &portinfo_) override;
    bool waitReceived(Duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling = false) override;
    bool waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line) override;
    void send(const QByteArray &data, const QByteArray &displayed_data = {}) override;
    void close() override;
    QString getName() override;

private:
    USBTMC usbtmc;
    bool is_connected;

};

#endif // USBTMCCOMMUNICATIONDEVICE_H
