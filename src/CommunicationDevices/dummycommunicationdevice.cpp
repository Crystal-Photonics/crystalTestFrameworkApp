#include "dummycommunicationdevice.h"


DummyCommunicationDevice::DummyCommunicationDevice() {
    name = "dummy";
    is_connected = false;
    emit connected(QByteArray());
}

bool DummyCommunicationDevice::connect(const QMap<QString, QVariant> &portinfo_)
{
    is_connected = true;
    this->portinfo = portinfo_;

    return true;
}

void DummyCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
    (void)displayed_data;
    (void)data;
}

bool DummyCommunicationDevice::isConnected() {

    return is_connected;
}

bool DummyCommunicationDevice::waitReceived(Duration timeout, int bytes, bool isPolling) {
    (void)timeout;
    (void)bytes;
    (void)isPolling;
    return true;
}

bool DummyCommunicationDevice::waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line){
    (void)timeout;
    (void)escape_characters;
    (void)leading_pattern_indicating_skip_line;
    return true;
}

void DummyCommunicationDevice::close() {
    is_connected = false;
}

QString DummyCommunicationDevice::getName()
{
    return name;
}
