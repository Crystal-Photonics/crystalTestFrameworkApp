#include "comportcommunicationdevice.h"
#include "qt_util.h"
#include "util.h"

#include <QApplication>
#include <QDebug>
#include <QString>
#include <cassert>
#include <regex>
#include <string>

ComportCommunicationDevice::ComportCommunicationDevice() {
    QObject::connect(&port, &QSerialPort::readyRead, [this]() {
        if (!currently_in_waitReceived) {
            emit waitReceived(std::chrono::seconds{0}, 0);
        }
    });
}

bool ComportCommunicationDevice::isConnected() {
    return Utility::promised_thread_call(this, [this] { return port.isOpen(); });
}

bool ComportCommunicationDevice::connect(const QMap<QString, QVariant> &portinfo_) {
    this->portinfo = portinfo_;
    return Utility::promised_thread_call(this, [this, portinfo_] {
        assert(portinfo_.contains(HOST_NAME_TAG));
        assert(portinfo_.contains(BAUD_RATE_TAG));
        assert(portinfo_[HOST_NAME_TAG].type() == QVariant::String);
        assert(portinfo_[BAUD_RATE_TAG].type() == QVariant::Int);

    //    qDebug() << QString("opening: ") + portinfo_[HOST_NAME_TAG].toString();
        port.setPortName(portinfo_[HOST_NAME_TAG].toString());
        port.setBaudRate(portinfo_[BAUD_RATE_TAG].toInt());
        bool result = port.open(QIODevice::ReadWrite);
        if (result) {
            QString protocol_name = portinfo_[TYPE_NAME_TAG].toString();
            QString text;
            if (protocol_name.count()) {
                text = protocol_name + ", ";
            }
            text += portinfo_[HOST_NAME_TAG].toString() + ", bd: " + portinfo_[BAUD_RATE_TAG].toString();
            auto ba = QByteArray();
            ba.append(text);
            emit connected(ba);
        } else {
            qDebug() << QString("could not open ") + portinfo_[HOST_NAME_TAG].toString();
        }
        return result;
    });
}

bool ComportCommunicationDevice::waitReceived(Duration timeout, int bytes, bool isPolling) {
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
    if (!isPolling) {
        currently_in_waitReceived = true;
    }
    do {
        try_read();
    } while (received_bytes < bytes &&
             std::chrono::high_resolution_clock::now() - now <=
                 timeout); //we want a <= because if we poll with 0ms we still put a heavy load on cpu(100% for 1ms each 16ms)
    try_read();
    if (!isPolling) {
        currently_in_waitReceived = false;
    }
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
    std::regex word_regex;
    try {
        word_regex = leading_pattern_indicating_skip_line;
    } catch (std::regex_error &e) {
        qDebug() << "faulty regex: " + QString().fromStdString(leading_pattern_indicating_skip_line);
        qDebug() << "error: " + QString().fromStdString(std::string(e.what()));
    }
    auto now = std::chrono::high_resolution_clock::now();
    do {
        QThread().currentThread()->usleep(100);
        if (port.bytesAvailable()) {
            //     now = std::chrono::high_resolution_clock::now();
        }
        try_read();
        if (inbuffer.indexOf(QString::fromStdString(escape_characters)) > -1) {
            emit received(inbuffer);
            escape_found = true;
            QString in_str{inbuffer};

            std::string in_line = in_str.toStdString();
            auto skipline_begin = std::sregex_iterator(in_line.begin(), in_line.end(), word_regex);
            auto skipline_end = std::sregex_iterator();
            int skipline_match = std::distance(skipline_begin, skipline_end);

            if ((!leading_pattern_indicating_skip_line.empty()) && skipline_match > 0) {
                //if ((!leading_pattern_indicating_skip_line.empty()) && in_str.startsWith(QString::fromStdString(leading_pattern_indicating_skip_line))){
                now = std::chrono::high_resolution_clock::now();
            } else {
                run = false;
            }
            inbuffer.clear();
        }
    } while (run && std::chrono::high_resolution_clock::now() - now <= timeout);
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
    return Utility::promised_thread_call(this, [this] { //
        //qDebug() << QString("closing: ") + portinfo[HOST_NAME_TAG].toString();
        port.close();
        QByteArray ar;
        ar.append(portinfo[HOST_NAME_TAG].toString());
        emit disconnected(ar);
    });
}

QString ComportCommunicationDevice::getName() {
    return port.portName();
}
