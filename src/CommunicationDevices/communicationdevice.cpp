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

bool CommunicationDevice::operator==(const QString &target) const {
	return this->target == target;
}

const QString &CommunicationDevice::getTarget() const {
    return target;
}

bool CommunicationDevice::is_waiting_for_message() const
{
    return currently_in_waitReceived;
}

void CommunicationDevice::set_currently_in_wait_received(bool in_wait_received)
{
    currently_in_waitReceived = in_wait_received;
}

void CommunicationDevice::set_is_in_use(bool in_use)
{
    this->in_use = in_use;
}

bool CommunicationDevice::get_is_in_use() const
{
    return in_use;
}

const QMap<QString, QVariant> &CommunicationDevice::get_port_info()
{
    return portinfo;
}





