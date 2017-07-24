#ifndef RPCSERIALPORT_H
#define RPCSERIALPORT_H


#include "rpcruntime_protcol.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QObject>

class RPCSerialPort : public RPCIODevice
{
    Q_OBJECT

public:
    RPCSerialPort();

    bool isConnected();
    bool connect(QString port_name, uint baud);
    bool waitReceived(std::chrono::steady_clock::duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling=false) override;

    void send(std::vector<unsigned char> data, std::vector<unsigned char> pre_encodec_data) override;
    void close();
    QString getName();

protected:
    bool currently_in_waitReceived = false;
signals:


private:
    void send(const QByteArray &data, const QByteArray &displayed_data = {});
    QSerialPort serial_port;
};

#endif // RPCSERIALPORT_H
