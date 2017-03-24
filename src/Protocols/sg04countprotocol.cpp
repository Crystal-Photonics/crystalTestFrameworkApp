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
        incoming_data.append(data);
        if (incoming_data.count() > 3) {
            for (int searching_offset = 0; searching_offset < 4; searching_offset++) {
                int offset = searching_offset;
                uint8_t package[4];
                bool ok = true;
                bool right_offset_found = false;
                while (ok) {
                    if (incoming_data.count() < 4 + offset) {
                        break;
                    }
                    for (int i = 0; i < 4; i++) {
                        package[i] = incoming_data[i + offset];
                    }

                    uint16_t counts = parse_sg04_count_package(package, ok);
                    if (ok) {
                        received_counts_gui += counts;
                        received_count_interval_gui++;

                        received_counts_lua += counts;
                        received_count_interval_lua++;
                        right_offset_found = true;
                        //qDebug() << "SG04-Count received" << counts;
                        incoming_data.remove(0, offset + 4);
                        offset = 0;
                    }
                }
                if (right_offset_found) {
                    break;
                }
            }
        }

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
}

sol::table SG04CountProtocol::get_sg04_counts(sol::state &lua, bool clear) {
    sol::table result = lua.create_table_with();
    result["counts"] = received_counts_lua;
    result["interval"] = received_count_interval_lua;
    if (clear){
        received_count_interval_lua = 0;
        received_counts_lua = 0;
    }
    return result;
}

int SG04CountProtocol::await_count_package()
{

}

bool SG04CountProtocol::is_correct_protocol() {
    incoming_data.clear();
    received_count_interval_gui = 0;
    received_counts_gui = 0;
    if (device->waitReceived(device_protocol_setting.timeout, 4, false)) {
        if (received_count_interval_gui) {
            return true;
        }
    }
    return false;
}
