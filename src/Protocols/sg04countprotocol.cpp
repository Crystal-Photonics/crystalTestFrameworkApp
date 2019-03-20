#include "sg04countprotocol.h"
#include "Windows/mainwindow.h"
#include <QDebug>
#include <assert.h>
const uint SG04_COUNT_INTERVAL_MS = 100;

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
    connection = QObject::connect(&device, &CommunicationDevice::received, [&device, this](const QByteArray &data) {
        incoming_data.append(data);
        if (incoming_data.count() > 3) {
            //  qDebug() << "bytes: "<<incoming_data.count() << "  elapsed[ms]: "<< performance_measurement_timer.elapsed();

            //  performance_measurement_timer.start();
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
                        actual_count_rate = counts;

                        received_counts += counts;
                        received_counts_mutex.lock();
                        received_count_packages.append(counts);
                        int max_count_entries = 1;
						if (device.is_in_use()) {
                            max_count_entries = 1000;
                        }
                        if (received_count_packages.count() > max_count_entries) {
                            received_count_packages.removeFirst();
                        }
                        received_counts_mutex.unlock();
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
    assert(QThread::currentThread() == MainWindow::gui_thread);
    ui_entry->setText(1, "SG04 Counts");
    ui_entry->setText(2, "");
}

void SG04CountProtocol::sg04_counts_clear_raw() {
    received_count_packages.clear();
    received_counts = 0;
}

void SG04CountProtocol::sg04_counts_clear() {
    received_counts_mutex.lock();
    sg04_counts_clear_raw();
    received_counts_mutex.unlock();
}

sol::table SG04CountProtocol::get_sg04_counts(sol::state &lua, bool clear) {
    sol::table result = lua.create_table_with();
    sol::table counts_table = lua.create_table_with();
    //  counts_table.
    received_counts_mutex.lock();
    for (auto i : received_count_packages) {
        counts_table.add(i);
    }
    received_counts_mutex.unlock();
    result["total"] = received_counts;
    result["counts"] = counts_table;
    if (clear) {
        sg04_counts_clear();
    }
    return result;
}

uint SG04CountProtocol::accumulate_counts(ScriptEngine *script_engine, uint time_ms) {
    assert(MainWindow::gui_thread != QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI

    if (time_ms % SG04_COUNT_INTERVAL_MS) {
        throw std::runtime_error(
            QString("SG04 accumulate_counts function: timeout must be multiple of %1 but is is %2").arg(SG04_COUNT_INTERVAL_MS).arg(time_ms).toStdString());
    }
    // qint64 starttime = QDateTime::currentMSecsSinceEpoch();
    sg04_counts_clear();
    uint result = 0;
    int timeout_interval = time_ms / SG04_COUNT_INTERVAL_MS;

    while (timeout_interval > 0) {
        script_engine->await_timeout(std::chrono::milliseconds{SG04_COUNT_INTERVAL_MS} * 2);
        received_counts_mutex.lock();
        for (auto i : received_count_packages) {
            result += i;
            timeout_interval--;
            if (timeout_interval == 0) {
                break;
            }
        }
        sg04_counts_clear_raw();
        received_counts_mutex.unlock();
    }
    //qDebug() << "elapsed time: " << QDateTime::currentMSecsSinceEpoch() - starttime;
    return result;
}

uint16_t SG04CountProtocol::get_actual_count_rate() {
    return actual_count_rate;
}

unsigned int SG04CountProtocol::get_actual_count_rate_cps() {
    return actual_count_rate * (1000 / SG04_COUNT_INTERVAL_MS);
}

bool SG04CountProtocol::is_correct_protocol() {
    incoming_data.clear();
    received_counts_mutex.lock();
    received_count_packages.clear();
    received_counts_mutex.unlock();
    if (device->waitReceived(device_protocol_setting.timeout, 4, false)) {
        received_counts_mutex.lock();
        auto count = received_count_packages.count();
        received_counts_mutex.unlock();
        if (count) {
            return true;
        }
    }
    return false;
}
