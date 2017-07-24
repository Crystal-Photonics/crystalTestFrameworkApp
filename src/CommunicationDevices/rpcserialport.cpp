#include "rpcserialport.h"
#include <assert.h>



RPCSerialPort::RPCSerialPort() {
    QObject::connect(&serial_port, &QSerialPort::readyRead, [this]() {
        if (!currently_in_waitReceived) {
            emit waitReceived(std::chrono::seconds{0}, 0);
        }
    });
}

bool RPCSerialPort::isConnected() {
    return  serial_port.isOpen();
}

bool RPCSerialPort::connect(QString port_name, uint baud) {
    serial_port.setPortName(port_name);
    serial_port.setBaudRate(baud);

    return serial_port.open(QIODevice::ReadWrite);

}

bool RPCSerialPort::waitReceived(std::chrono::steady_clock::duration timeout, int bytes, bool isPolling) {
    auto now = std::chrono::high_resolution_clock::now();
    int received_bytes = 0;
    auto try_read = [this, &received_bytes] {
       // auto result = Utility::promised_thread_call(this,  [this] {
         //   QApplication::processEvents();
        QByteArray result = serial_port.readAll();
        //});
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

void RPCSerialPort::send(std::vector<unsigned char> data, std::vector<unsigned char> pre_encodec_data)
{
    QByteArray ba_data;
    for (auto d:data){
        ba_data.append(d);
    }

    QByteArray ba_pre_enc;
    for (auto d:pre_encodec_data){
        ba_pre_enc.append(d);
    }
    send(ba_data,ba_pre_enc);
}

void RPCSerialPort::send(const QByteArray &data, const QByteArray &displayed_data) {

    auto size = serial_port.write(data);
    if (size == -1) {
        return;
    }
    if (size != data.size()) {
        size += serial_port.write(data.data() + size, data.size() - size);
        assert(size == data.size());
    }
    emit decoded_sent(displayed_data.isEmpty() ? data : displayed_data);
    emit sent(data);

}

void RPCSerialPort::close() {
    serial_port.close();
}

QString RPCSerialPort::getName() {
    return serial_port.portName();
}
