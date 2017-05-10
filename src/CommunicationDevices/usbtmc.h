#ifndef USBTMC_H
#define USBTMC_H
#include "CommunicationDevices/usbtmc_libusb.h"
#include <chrono>

class USBTMC {
    public:
    using Duration = std::chrono::steady_clock::duration;
    USBTMC();
    ~USBTMC();

    void open(QString id);
    void close();
    void send_buffer(const QByteArray &data);
    QByteArray read_answer();
    void set_timeout(Duration timeout);

    private:
    struct scpi_usbtmc_libusb uscpi;

    int sr_scpi_read_response(QByteArray &response,  std::chrono::high_resolution_clock::time_point timeout_until);

    Duration timeout;
};

#endif // USBTMC_H
