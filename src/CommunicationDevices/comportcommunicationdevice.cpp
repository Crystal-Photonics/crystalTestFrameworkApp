#include "comportcommunicationdevice.h"
#include "qt_util.h"

#include <QApplication>
#include <cassert>

ComportCommunicationDevice::ComportCommunicationDevice(QString target) {
    this->target = target;
}

bool ComportCommunicationDevice::isConnected() {
    return Utility::promised_thread_call(this, [this] { return port.isOpen(); });
}

bool ComportCommunicationDevice::connect(const QSerialPortInfo &portinfo, QSerialPort::BaudRate baudrate) {
    return Utility::promised_thread_call(this, [this, portinfo, baudrate] {
        port.setPort(portinfo);
        port.setBaudRate(baudrate);
        return port.open(QIODevice::ReadWrite);
    });
}

bool ComportCommunicationDevice::waitReceived(Duration timeout, int bytes) {
    auto now = std::chrono::high_resolution_clock::now();
    int received_bytes = 0;
    auto try_read = [this, &received_bytes] {
        auto result = Utility::promised_thread_call(this, [this] {
            QApplication::processEvents();
            return port.readAll();
        });
        if (result.isEmpty() == false) {
            emit received(result);
            received_bytes += result.size();
        }
    };
    currently_in_waitReceived = true;
    do {
        try_read();
    } while (received_bytes < bytes && std::chrono::high_resolution_clock::now() - now < timeout);
    try_read();
    currently_in_waitReceived = false;
    return received_bytes >= bytes;
}

bool ComportCommunicationDevice::waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line) {
    QByteArray inbuffer{};

    auto try_read = [this, &inbuffer] {
        auto result = Utility::promised_thread_call(this, [this] {
            QApplication::processEvents();
            char byte_buffer = 0;
            QByteArray inbyte_buffer{};
            if (port.read(&byte_buffer, 1)) {
                inbyte_buffer.append(byte_buffer);
            }
            return inbyte_buffer;
        });
        if (result.isEmpty() == false) {
            inbuffer.append(result);
        }
    };
    currently_in_waitReceived = true;
    bool escape_found = false;
    bool run = true;
    auto now = std::chrono::high_resolution_clock::now();
    do {
        if (port.bytesAvailable()){
       //     now = std::chrono::high_resolution_clock::now();
        }
        try_read();
        if (inbuffer.indexOf(QString::fromStdString(escape_characters)) > -1) {
            emit received(inbuffer);
            escape_found = true;
            QString in_str{inbuffer};

            if ((!leading_pattern_indicating_skip_line.empty()) && in_str.startsWith(QString::fromStdString(leading_pattern_indicating_skip_line))){
                now = std::chrono::high_resolution_clock::now();
            } else {
                run = false;
            }
            inbuffer.clear();
        }
    } while (run && std::chrono::high_resolution_clock::now() - now < timeout);
  //  try_read();
    currently_in_waitReceived = false;

    return escape_found;
}

void ComportCommunicationDevice::send(const QByteArray &data, const QByteArray &displayed_data) {
    return Utility::promised_thread_call(this, [this, data, displayed_data] {
        auto size = port.write(data);
        if (size == -1) {
            return;
        }
        if (size != data.size()) {
            size += port.write(data.data() + size, data.size() - size);
            assert(size == data.size());
        }
        emit decoded_sent(displayed_data.isEmpty() ? data : displayed_data);
        emit sent(data);
    });
}

void ComportCommunicationDevice::close() {
    return Utility::promised_thread_call(this, [this] { port.close(); });
}
