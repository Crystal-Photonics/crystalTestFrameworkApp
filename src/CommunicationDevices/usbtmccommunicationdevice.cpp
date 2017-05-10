#include "usbtmccommunicationdevice.h"
#include "assert.h"
#include "qt_util.h"
#include "util.h"
#include <QDebug>

/* Some USBTMC-specific enums, as defined in the USBTMC standard. */
#define SUBCLASS_USBTMC 0x03
#define USBTMC_USB488 0x01

USBTMCCommunicationDevice::USBTMCCommunicationDevice() {
    is_connected = false;
}

USBTMCCommunicationDevice::~USBTMCCommunicationDevice() {
    if (is_connected) {
        usbtmc.close();
    }
}

bool USBTMCCommunicationDevice::isConnected() {
    return is_connected;
}

bool USBTMCCommunicationDevice::connect(const QMap<QString, QVariant> &portinfo) {
    usbtmc.open(portinfo[HOST_NAME_TAG].toString());
    is_connected = true;
    return true;
}

bool USBTMCCommunicationDevice::waitReceived(CommunicationDevice::Duration timeout, int bytes, bool isPolling) {
    assert(0); //not implemented
    return true;
}

bool USBTMCCommunicationDevice::waitReceived(CommunicationDevice::Duration timeout, std::string escape_characters,
                                             std::string leading_pattern_indicating_skip_line) {
    usbtmc.set_timeout(timeout);
    QByteArray response = usbtmc.read_answer();
    if (response.size()) {
        emit received(response);
        return true;
    }else{
        return false;
    }

}

void USBTMCCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
#if 0
    return Utility::promised_thread_call(this, [this, data, displayed_data] {


        emit sent(data);
    });
#endif

    usbtmc.send_buffer(data);
    emit decoded_sent(displayed_data.isEmpty() ? data : displayed_data);
}

void USBTMCCommunicationDevice::close() {
    is_connected = false;
    usbtmc.close();
}

QString USBTMCCommunicationDevice::getName() {
    return portinfo[HOST_NAME_TAG].toString();
}
