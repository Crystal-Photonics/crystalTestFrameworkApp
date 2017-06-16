#include "usbtmc.h"

#include "libusb-1.0/libusb.h"
#include <QDebug>
#include <assert.h>

USBTMC::USBTMC() {
    int r = libusb_init(&uscpi.ctx);
    if (r < 0) {
        qDebug() << QString("could not initialize lib usb. (%1)").arg(QString::number(r));
    }
}

USBTMC::~USBTMC() {
    libusb_exit(uscpi.ctx);
}

bool USBTMC::open(QString id) {
    scpi_usbtmc_libusb_dev_inst_new(&uscpi, id);
    scpi_usbtmc_libusb_open(&uscpi);
    return uscpi.usb.devhdl;
}

void USBTMC::close() {
    scpi_usbtmc_libusb_close(&uscpi);
}

void USBTMC::send_buffer(const QByteArray &data) {
    assert(uscpi.usb.devhdl);
    scpi_usbtmc_libusb_send(&uscpi, data);
}

/**
 * Do a non-blocking read of up to the allocated length, and
 * check if a timeout has occured.
 *
 * @param scpi Previously initialised SCPI device structure.
 * @param response Buffer to which the response is appended.
 * @param abs_timeout_us Absolute timeout in microseconds
 *
 * @return read length on success, SR_ERR* on failure.
 */

int USBTMC::sr_scpi_read_response(QByteArray &response, std::chrono::high_resolution_clock::time_point timeout_until) {
    char buffer[MAX_TRANSFER_LENGTH];

    int len = scpi_usbtmc_libusb_read_data(&uscpi, buffer, MAX_TRANSFER_LENGTH);

    if (len < 0) {
        sr_err("Incompletely read SCPI response.");
        return SR_ERR;
    }

    if (len > 0) {
        response.append(buffer, len);
        return len;
    }

    if (std::chrono::high_resolution_clock::now() > timeout_until) {
        sr_err("Timed out waiting for SCPI response.");
        return SR_ERR_TIMEOUT;
    }

    return 0;
}

QByteArray USBTMC::read_answer() {
    assert(uscpi.usb.devhdl);
    int ret;
    QByteArray response;

    if (scpi_usbtmc_libusb_read_begin(&uscpi) != SR_OK) {
        return {};
        //SR_ERR;
    }

    auto timeout_until = std::chrono::high_resolution_clock::now() + this->timeout;

    while (!scpi_usbtmc_libusb_read_complete(&uscpi)) {
        /* Read another chunk of the response. */
        ret = sr_scpi_read_response(response, timeout_until);

        if (ret < 0) {
            return {};
        }
        if (ret > 0) {
            timeout_until = std::chrono::high_resolution_clock::now() + this->timeout;
        }
    }

    return response;
}

void USBTMC::set_timeout(USBTMC::Duration timeout) {
    this->timeout = timeout;
}
