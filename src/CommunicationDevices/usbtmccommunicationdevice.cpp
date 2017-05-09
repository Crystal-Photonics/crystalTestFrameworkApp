#include "usbtmccommunicationdevice.h"

#include <QDebug>

/* Some USBTMC-specific enums, as defined in the USBTMC standard. */
#define SUBCLASS_USBTMC 0x03
#define USBTMC_USB488 0x01

USBTMCCommunicationDevice::USBTMCCommunicationDevice() {}

USBTMCCommunicationDevice::~USBTMCCommunicationDevice() {}






bool USBTMCCommunicationDevice::isConnected() {
    return true;
}

bool USBTMCCommunicationDevice::connect(const QMap<QString, QVariant> &portinfo) {
    return true;
}

bool USBTMCCommunicationDevice::waitReceived(CommunicationDevice::Duration timeout, int bytes, bool isPolling) {
    return true;
}

bool USBTMCCommunicationDevice::waitReceived(CommunicationDevice::Duration timeout, std::string escape_characters,
                                             std::string leading_pattern_indicating_skip_line) {
    return true;
}

void USBTMCCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {}

void USBTMCCommunicationDevice::close() {}

QString USBTMCCommunicationDevice::getName() {
    return "sdfsd";
}
