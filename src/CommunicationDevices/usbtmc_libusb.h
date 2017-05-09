#ifndef usbtmc_libusb_H
#define usbtmc_libusb_H

#include <QStringList>
#include <QByteArray>
#include "libusb_base.h"

#define MAX_TRANSFER_LENGTH 2048

struct scpi_usbtmc_libusb {
    libusb_context *ctx;
    struct sr_usb_dev_inst usb;
    int detached_kernel_driver;
    uint8_t interface;
    uint8_t bulk_in_ep;
    uint8_t bulk_out_ep;
    uint8_t interrupt_ep;
    uint8_t usbtmc_int_cap;
    uint8_t usbtmc_dev_cap;
    uint8_t usb488_dev_cap;
    uint8_t bTag;
    uint8_t bulkin_attributes;
    uint8_t buffer[MAX_TRANSFER_LENGTH];
    int response_length;
    int response_bytes_read;
    int remaining_length;
};

QStringList list_usb_tmc_devices();
int scpi_usbtmc_libusb_dev_inst_new(struct scpi_usbtmc_libusb *uscpi, QString params);
int scpi_usbtmc_libusb_open(scpi_usbtmc_libusb *uscpi);
int scpi_usbtmc_libusb_close(scpi_usbtmc_libusb *uscpi);
int scpi_usbtmc_libusb_send(struct scpi_usbtmc_libusb *uscpi, const QByteArray &command);
int scpi_usbtmc_libusb_read_begin(struct scpi_usbtmc_libusb *uscpi);
int scpi_usbtmc_libusb_read_complete(struct scpi_usbtmc_libusb *uscpi);
int scpi_usbtmc_libusb_read_data(struct scpi_usbtmc_libusb *uscpi, char *buf, int maxlen);
#endif
