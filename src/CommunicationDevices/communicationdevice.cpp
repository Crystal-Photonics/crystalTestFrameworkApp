#include "communicationdevice.h"
#include "CommunicationDevices/comportcommunicationdevice.h"
#include "echocommunicationdevice.h"
#include "socketcommunicationdevice.h"

#include <QDebug>
#include <regex>

void CommunicationDevice::send(const std::vector<unsigned char> &data, const std::vector<unsigned char> &displayed_data) {
    size_t datasize = data.size();
    send(QByteArray(reinterpret_cast<const char *>(data.data()), datasize),
         QByteArray(reinterpret_cast<const char *>(displayed_data.data()), displayed_data.size()));
}

QString CommunicationDevice::get_identifier_display_string() const {
    if (portinfo.contains(HOST_NAME_TAG)) {
        QString result = portinfo[HOST_NAME_TAG].toString();
        if (result == "manual") {
			result = portinfo[DEVICE_MANUAL_NAME_TAG].toString() + " (" + portinfo[DEVICE_MANUAL_SN_TAG].toString() + ")";
        }
        return result;
    }
    return "";
}

bool CommunicationDevice::is_waiting_for_message() const {
	return currently_in_waitReceived;
}

void CommunicationDevice::set_currently_in_wait_received(bool in_wait_received) {
	currently_in_waitReceived = in_wait_received;
}

bool CommunicationDevice::is_in_use() const {
	return in_use;
}

const QMap<QString, QVariant> &CommunicationDevice::get_port_info() {
    return portinfo;
}
