#include "sg04countprotocol.h"
#include <QDebug>
#include <assert.h>

static uint16_t valid_sg04_count_package(QByteArray &indata){

}

SG04CountProtocol::SG04CountProtocol(CommunicationDevice &device, DeviceProtocolSetting &setting)
    : Protocol{"SG04Count"}
    , device(&device)
    , device_protocol_setting(setting) {
#if 1
    connection = QObject::connect(&device, &CommunicationDevice::received, [this](const QByteArray &data) {
//qDebug() << "SG04-Count received" << data.size() << "bytes from device";
//qDebug() << "SG04-Count received" << data << "bytes from device";
#if 1
        incoming_data.append(data);
        if (incoming_data.count() > 3) {
            for (int searching_offset = 0; searching_offset < 4; searching_offset++) {
                if (incoming_data.count() < 4 + searching_offset) {
                    break;
                }
                if (((uint8_t)incoming_data.at(searching_offset + 0) == 0xAA) && ((uint8_t)incoming_data.at(searching_offset + 3) == 0xAA)) {
                    while (incoming_data.count() > 3) {
                        uint16_t counts = 0;
                        counts = incoming_data.at(searching_offset + 2);
                        counts |= incoming_data.at(searching_offset + 1) << 8;
                        received_counts += counts;
                        received_count_interval++;
                        qDebug() << "SG04-Count received" << counts;
                        incoming_data.remove(0, searching_offset + 4);

                    }
                }
            }
        }
#endif
    });
#endif
    assert(connection);
}

SG04CountProtocol::~SG04CountProtocol() {
    assert(connection);
    auto result = QObject::disconnect(connection);
    assert(result);
}

void SG04CountProtocol::set_ui_description(QTreeWidgetItem *ui_entry) {
    ui_entry->setText(1, "SG04 Counts");
    ui_entry->setText(2, "");
    //  set_display_data(ui_entry, device_data);
}

bool SG04CountProtocol::is_correct_protocol() {
    incoming_data.clear();
    received_count_interval = 0;
    received_counts = 0;
    if (device->waitReceived(device_protocol_setting.timeout, 4, false)) {
        if (received_count_interval) {
            return true;
        }
    }
    return false;
}
