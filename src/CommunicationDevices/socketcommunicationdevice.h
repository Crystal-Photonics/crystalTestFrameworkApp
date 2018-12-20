#ifndef SOCKETCOMMUNICATIONDEVICE_H
#define SOCKETCOMMUNICATIONDEVICE_H

#include "communicationdevice.h"
#include "export.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <functional>
#include <memory>

class EXPORT SocketCommunicationDevice final : public CommunicationDevice {
    public:
    SocketCommunicationDevice();
    void send(const QByteArray &data, const QByteArray &displayed_data = {}) override;
    ~SocketCommunicationDevice();
    bool awaitConnection(Duration timeout = std::chrono::seconds(1));
    bool waitReceived(Duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling = false) override;
    bool waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line) override;
    void close() override;
    bool connect(const QMap<QString,QVariant> &portinfo_) override;

    private:
    QTcpSocket *socket; //QTcpSocket does not support move semantics and is somehow auto-deleted, probably
    QTcpServer server;
    void setSocket();
    QMetaObject::Connection connectedSlot, receiveSlot;
    std::function<void()> callSetSocket;
    bool isConnected() override;
    void receiveData(QByteArray data);
    bool isServer;
    QString getName() override;
};

#endif // SOCKETCOMMUNICATIONDEVICE_H
