#include "socketcommunicationdevice.h"
#include "util.h"
#include <QDebug>
#include <cassert>
#include <exception>
#include <memory>
#include <regex>

SocketCommunicationDevice::SocketCommunicationDevice()
    : socket(nullptr) {
#if 1
    //std::regex ipPort(R"(((server:)|(client:))([[:digit:]]{1,3}\.){3}[[:digit:]]{1,3}:[[:digit:]]{1,5})");
    //auto success = std::regex_match(target.toStdString(), ipPort);
    bool success = true;
    assert(success);
    //auto typeIpPort = target.split(':');
    auto type = QString{"client"};
    auto ip = QString{"localhost"};
    int port;
    success = Utility::convert(QString{"111"}, port);
    assert(success);
    server.setMaxPendingConnections(1);
    if (type == "client") {
        isServer = false;
        socket = new QTcpSocket;
        socket->connectToHost(ip, port);
        /* Fehler: invalid use of non-static member function 'void QIODevice::readyRead()'
   receiveSlot = connect(socket, QTcpSocket::readyRead, [this]() { this->receiveData(this->socket->readAll()); });
                                             ^
                                             */
        receiveSlot = QObject::connect(socket, &QTcpSocket::readyRead, [this]() { this->receiveData(this->socket->readAll()); });
        assert(receiveSlot);
    } else if (type == "server") {
        isServer = true;
        auto success = server.listen(QHostAddress(ip), port);
        if (!success)
            throw std::runtime_error("Failed opening " + ip.toStdString() + ':' + std::to_string(port));
        callSetSocket = [this]() {
            this->setSocket();
            this->connected(QByteArray(""));
        };
        connectedSlot = QObject::connect(&server, &QTcpServer::newConnection, callSetSocket);
        assert(connectedSlot);
    } else {
        throw std::logic_error("regex logic is wrong: " + type.toStdString() + " must be \"server\" or \"client\"");
    }
#endif
}

void SocketCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
    (void)displayed_data;
    socket->write(data);
    socket->waitForBytesWritten(1000);
}

SocketCommunicationDevice::~SocketCommunicationDevice() {
    QObject::disconnect(connectedSlot);
    QObject::disconnect(receiveSlot);
}

bool SocketCommunicationDevice::awaitConnection(Duration timeout) {
    if (isServer) {
        return server.waitForNewConnection(timeout.count());
    }
    return socket->waitForConnected(timeout.count());
}

bool SocketCommunicationDevice::waitReceived(Duration timeout, int bytes, bool isPolling) {
    (void)bytes; //TODO: fix it so it waits for [bytes] until [timeout]
    (void)isPolling;
    return socket->waitForReadyRead(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
}

bool SocketCommunicationDevice::waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line) {
    (void)escape_characters;
    (void)leading_pattern_indicating_skip_line;
    return socket->waitForReadyRead(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
}

void SocketCommunicationDevice::close() {
    socket->close();
}

bool SocketCommunicationDevice::connect(const QMap<QString, QVariant> &portinfo_) {
    this->portinfo = portinfo_;
    return true;
}

void SocketCommunicationDevice::setSocket() {
    socket = server.nextPendingConnection();
    if (receiveSlot)
        QObject::disconnect(receiveSlot);
    receiveSlot = QObject::connect(socket, &QTcpSocket::readyRead, [this]() { this->receiveData(this->socket->readAll()); });
    assert(receiveSlot);
}

bool SocketCommunicationDevice::isConnected() {
    if (!socket)
        return false;
    return socket->state() == QAbstractSocket::ConnectedState;
}

void SocketCommunicationDevice::receiveData(QByteArray data) {
    emit received(std::move(data));
}

QString SocketCommunicationDevice::getName() {
    return "TCP/IP";
}
