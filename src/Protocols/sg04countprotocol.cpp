#include "sg04countprotocol.h"
#include <QDebug>
#include <assert.h>

static uint16_t parse_sg04_count_package(uint8_t indata[4], bool &ok) {
    uint16_t result = 0;
    ok = false;
    if ((indata[0] == 0xAA) && (indata[3] == 0xAA)) {
        result = indata[2];
        result |= indata[1] << 8;
        ok = true;
    }
    return result;
}

SG04CountProtocol::SG04CountProtocol(CommunicationDevice &device, DeviceProtocolSetting &setting)
    : Protocol{"SG04Count"}
    , device(&device)
    , device_protocol_setting(setting) {
#if 1
    connection = QObject::connect(&device, &CommunicationDevice::received, [this](const QByteArray &data) {

#if 1
        incoming_data.append(data);
        if (incoming_data.count() > 3) {
            for (int searching_offset = 0; searching_offset < 4; searching_offset++) {
                int offset = searching_offset;
                uint8_t package[4];

                while (ok) {
                    if (incoming_data.count() < 4 + offset) {
                        break;
                    }
                    for (int i = 0; i < 4; i++) {
                        package[i] = incoming_data[i + offset];
                    }
                    bool ok = false;
                    uint16_t counts = parse_sg04_count_package(package, ok);
                    if (ok) {
                        received_counts += counts;
                        received_count_interval++;
                        qDebug() << "SG04-Count received" << counts;
                        incoming_data.remove(0, offset + 4);
                        offset = 0;
                    } else {
                        break;
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
