/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2014 Aurelien Jacobs <aurel@gnuage.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if 1
#include "usbtmc_libusb.h"
#include "libusb-1.0/libusb.h"
#include "libusb_base.h"
//#include <config.h>
//#include <string.h>
//#include <libsigrok/libsigrok.h>
//#include "libsigrok-internal.h"
//#include "scpi.h"

#include <QDebug>
#include <inttypes.h>

#define LOG_PREFIX "scpi_usbtmc"

#define TRANSFER_TIMEOUT 1000

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * Write a 8 bits unsigned integer to memory.
 * @param p a pointer to the output memory
 * @param x the input unsigned integer
 */
#define W8(p, x)                                                                                                                                               \
    do {                                                                                                                                                       \
        ((uint8_t *)(p))[0] = (uint8_t)(x);                                                                                                                    \
    } while (0)

/**
 * Write a 16 bits unsigned integer to memory stored as little endian.
 * @param p a pointer to the output memory
 * @param x the input unsigned integer
 */
#define WL16(p, x)                                                                                                                                             \
    do {                                                                                                                                                       \
        ((uint8_t *)(p))[0] = (uint8_t)(x);                                                                                                                    \
        ((uint8_t *)(p))[1] = (uint8_t)((x) >> 8);                                                                                                             \
    } while (0)

/**
 * Write a 32 bits unsigned integer to memory stored as little endian.
 * @param p a pointer to the output memory
 * @param x the input unsigned integer
 */
#define WL32(p, x)                                                                                                                                             \
    do {                                                                                                                                                       \
        ((uint8_t *)(p))[0] = (uint8_t)(x);                                                                                                                    \
        ((uint8_t *)(p))[1] = (uint8_t)((x) >> 8);                                                                                                             \
        ((uint8_t *)(p))[2] = (uint8_t)((x) >> 16);                                                                                                            \
        ((uint8_t *)(p))[3] = (uint8_t)((x) >> 24);                                                                                                            \
    } while (0)

/**
 * Read a 8 bits unsigned integer out of memory.
 * @param x a pointer to the input memory
 * @return the corresponding unsigned integer
 */
#define R8(x) ((unsigned)((const uint8_t *)(x))[0])

/**
 * Read a 32 bits little endian unsigned integer out of memory.
 * @param x a pointer to the input memory
 * @return the corresponding unsigned integer
 */
#define RL32(x)                                                                                                                                                \
    (((unsigned)((const uint8_t *)(x))[3] << 24) | ((unsigned)((const uint8_t *)(x))[2] << 16) | ((unsigned)((const uint8_t *)(x))[1] << 8) |                  \
     (unsigned)((const uint8_t *)(x))[0])

/* Static definitions of structs ending with an all-zero entry are a
 * problem when compiling with -Wmissing-field-initializers: GCC
 * suppresses the warning only with { 0 }, clang wants { } */
#ifdef __clang__
#define ALL_ZERO                                                                                                                                               \
    {}
#else
#define ALL_ZERO                                                                                                                                               \
    { 0 }
#endif

/* Some USBTMC-specific enums, as defined in the USBTMC standard. */
#define SUBCLASS_USBTMC 0x03
#define USBTMC_USB488 0x01

enum {
    /* USBTMC control requests */
    INITIATE_ABORT_BULK_OUT = 1,
    CHECK_ABORT_BULK_OUT_STATUS = 2,
    INITIATE_ABORT_BULK_IN = 3,
    CHECK_ABORT_BULK_IN_STATUS = 4,
    INITIATE_CLEAR = 5,
    CHECK_CLEAR_STATUS = 6,
    GET_CAPABILITIES = 7,
    INDICATOR_PULSE = 64,

    /* USB488 control requests */
    READ_STATUS_BYTE = 128,
    REN_CONTROL = 160,
    GO_TO_LOCAL = 161,
    LOCAL_LOCKOUT = 162,
};

/* USBTMC status codes */
#define USBTMC_STATUS_SUCCESS 0x01

/* USBTMC capabilities */
#define USBTMC_INT_CAP_LISTEN_ONLY 0x01
#define USBTMC_INT_CAP_TALK_ONLY 0x02
#define USBTMC_INT_CAP_INDICATOR 0x04

#define USBTMC_DEV_CAP_TERMCHAR 0x01

#define USB488_DEV_CAP_DT1 0x01
#define USB488_DEV_CAP_RL1 0x02
#define USB488_DEV_CAP_SR1 0x04
#define USB488_DEV_CAP_SCPI 0x08

/* Bulk messages constants */
#define USBTMC_BULK_HEADER_SIZE 12

/* Bulk MsgID values */
#define DEV_DEP_MSG_OUT 1
#define REQUEST_DEV_DEP_MSG_IN 2
#define DEV_DEP_MSG_IN 2

/* bmTransferAttributes */
#define EOM 0x01
#define TERM_CHAR_ENABLED 0x02

struct usbtmc_blacklist {
    uint16_t vid;
    uint16_t pid;
};

/* Devices that publish RL1 support, but don't support it. */
static struct usbtmc_blacklist blacklist_remote[] = {{0x1ab1, 0x0588}, /* Rigol DS1000 series */
                                                     {0x1ab1, 0x04b0}, /* Rigol DS2000 series */
                                                     {0x0957, 0x0588}, /* Agilent DSO1000 series (rebadged Rigol DS1000) */
                                                     {0x0b21, 0xffff}, /* All Yokogawa devices */
                                                     ALL_ZERO};

QStringList list_usb_tmc_devices() {
    QStringList result;

    struct libusb_device **devlist;
    struct libusb_device_descriptor des;
    struct libusb_config_descriptor *confdes;
    const struct libusb_interface_descriptor *intfdes;

    int confidx, intfidx, ret, i;

    ret = libusb_get_device_list(NULL, &devlist);
    if (ret < 0) {
        qDebug() << QString("Failed to get device list: %s.").arg(QString::fromLocal8Bit(libusb_error_name(ret)));
        return {};
    }
    for (i = 0; devlist[i]; i++) {
        libusb_get_device_descriptor(devlist[i], &des);

        for (confidx = 0; confidx < des.bNumConfigurations; confidx++) {
            if ((ret = libusb_get_config_descriptor(devlist[i], confidx, &confdes)) < 0) {
                qDebug() << QString("Failed to get configuration descriptor: %1, ignoring device.").arg(QString::fromLocal8Bit(libusb_error_name(ret)));
                break;
            }
            for (intfidx = 0; intfidx < confdes->bNumInterfaces; intfidx++) {
                intfdes = confdes->interface[intfidx].altsetting;
                if (intfdes->bInterfaceClass != LIBUSB_CLASS_APPLICATION || intfdes->bInterfaceSubClass != SUBCLASS_USBTMC ||
                    intfdes->bInterfaceProtocol != USBTMC_USB488) {
                    continue;
                }
                qDebug() << QString("Found USBTMC device (VID:PID = %1:%2, bus.address = %3.%4).")
                                .arg(QString::number(des.idVendor, 16))
                                .arg(QString::number(des.idProduct, 16))
                                .arg(libusb_get_bus_number(devlist[i]))
                                .arg(libusb_get_device_address(devlist[i]));

                QString res = QString("usbtmc/%1.%2").arg(libusb_get_bus_number(devlist[i])).arg(libusb_get_device_address(devlist[i]));
                result.append(res);
            }
            libusb_free_config_descriptor(confdes);
        }
    }
    libusb_free_device_list(devlist, 1);

    return result;
}

int scpi_usbtmc_libusb_dev_inst_new(struct scpi_usbtmc_libusb *uscpi, QString params) {

    //uscpi->ctx = drvc;
    //can be of the form "<bus>.<address>", or "<vendorid>:<productid>".

    QList<sr_usb_dev_inst> devices = sr_usb_find(uscpi->ctx, params);
    if (devices.size() != 1) {
        sr_err("Failed to find USB device '%s'.", params);
        return SR_ERR;
    }
    uscpi->usb = devices[0];

    return SR_OK;
}

static int check_usbtmc_blacklist(struct usbtmc_blacklist *blacklist, uint16_t vid, uint16_t pid) {
    int i;

    for (i = 0; blacklist[i].vid; i++) {
        if ((blacklist[i].vid == vid && blacklist[i].pid == 0xFFFF) || (blacklist[i].vid == vid && blacklist[i].pid == pid))
            return TRUE;
    }

    return FALSE;
}

static int scpi_usbtmc_remote(struct scpi_usbtmc_libusb *uscpi) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    struct libusb_device *dev;
    struct libusb_device_descriptor des;
    int ret;
    uint8_t status;

    if (!(uscpi->usb488_dev_cap & USB488_DEV_CAP_RL1))
        return SR_OK;

    dev = libusb_get_device(usb->devhdl);
    libusb_get_device_descriptor(dev, &des);
    if (check_usbtmc_blacklist(blacklist_remote, des.idVendor, des.idProduct))
        return SR_OK;

    sr_dbg("Locking out local control.");
    ret = libusb_control_transfer(usb->devhdl, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, REN_CONTROL, 1, uscpi->interface,
                                  &status, 1, TRANSFER_TIMEOUT);
    if (ret < 0 || status != USBTMC_STATUS_SUCCESS) {
        if (ret < 0)
            sr_dbg("Failed to enter REN state: %s.", libusb_error_name(ret));
        else
            sr_dbg("Failed to enter REN state: USBTMC status %d.", status);
        return SR_ERR;
    }

    ret = libusb_control_transfer(usb->devhdl, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, LOCAL_LOCKOUT, 0, uscpi->interface,
                                  &status, 1, TRANSFER_TIMEOUT);
    if (ret < 0 || status != USBTMC_STATUS_SUCCESS) {
        if (ret < 0)
            sr_dbg("Failed to enter local lockout state: %s.", libusb_error_name(ret));
        else
            sr_dbg(
                "Failed to enter local lockout state: USBTMC "
                "status %d.",
                status);
        return SR_ERR;
    }

    return SR_OK;
}

static void scpi_usbtmc_local(struct scpi_usbtmc_libusb *uscpi) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    struct libusb_device *dev;
    struct libusb_device_descriptor des;
    int ret;
    uint8_t status;

    if (!(uscpi->usb488_dev_cap & USB488_DEV_CAP_RL1))
        return;

    dev = libusb_get_device(usb->devhdl);
    libusb_get_device_descriptor(dev, &des);
    if (check_usbtmc_blacklist(blacklist_remote, des.idVendor, des.idProduct))
        return;

    sr_dbg("Returning local control.");
    ret = libusb_control_transfer(usb->devhdl, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, GO_TO_LOCAL, 0, uscpi->interface,
                                  &status, 1, TRANSFER_TIMEOUT);
    if (ret < 0 || status != USBTMC_STATUS_SUCCESS) {
        if (ret < 0)
            sr_dbg("Failed to clear local lockout state: %s.", libusb_error_name(ret));
        else
            sr_dbg(
                "Failed to clear local lockout state: USBTMC "
                "status %d.",
                status);
    }

    return;
}

int scpi_usbtmc_libusb_open(struct scpi_usbtmc_libusb *uscpi) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    struct libusb_device_descriptor des;

    int  config = 0;
    uint8_t capabilities[24];
    int ret, found = 0;

    if (usb->devhdl)
        return SR_OK;

    if (sr_usb_open(uscpi->ctx, usb) != SR_OK)
        return SR_ERR;

    struct libusb_device *dev = libusb_get_device(usb->devhdl);
    libusb_get_device_descriptor(dev, &des);

    for (int confidx = 0; confidx < des.bNumConfigurations; confidx++) {
        struct libusb_config_descriptor *confdes;
        if ((ret = libusb_get_config_descriptor(dev, confidx, &confdes)) < 0) {
            sr_dbg(
                "Failed to get configuration descriptor: %s, "
                "ignoring device.",
                libusb_error_name(ret));
            continue;
        }
        for (int intfidx = 0; intfidx < confdes->bNumInterfaces; intfidx++) {
            const struct libusb_interface_descriptor *intfdes = confdes->interface[intfidx].altsetting;
            if (intfdes->bInterfaceClass != LIBUSB_CLASS_APPLICATION || intfdes->bInterfaceSubClass != SUBCLASS_USBTMC ||
                intfdes->bInterfaceProtocol != USBTMC_USB488)
                continue;
            uscpi->interface = intfdes->bInterfaceNumber;
            config = confdes->bConfigurationValue;
            sr_dbg("Interface %d configuration %d.", uscpi->interface, config);
            for (int epidx = 0; epidx < intfdes->bNumEndpoints; epidx++) {
                const struct libusb_endpoint_descriptor *ep = &intfdes->endpoint[epidx];
                if (ep->bmAttributes == LIBUSB_TRANSFER_TYPE_BULK && !(ep->bEndpointAddress & (LIBUSB_ENDPOINT_DIR_MASK))) {
                    uscpi->bulk_out_ep = ep->bEndpointAddress;
                    sr_dbg("Bulk OUT EP %d", uscpi->bulk_out_ep);
                }
                if (ep->bmAttributes == LIBUSB_TRANSFER_TYPE_BULK && ep->bEndpointAddress & (LIBUSB_ENDPOINT_DIR_MASK)) {
                    uscpi->bulk_in_ep = ep->bEndpointAddress;
                    sr_dbg("Bulk IN EP %d", uscpi->bulk_in_ep & 0x7f);
                }
                if (ep->bmAttributes == LIBUSB_TRANSFER_TYPE_INTERRUPT && ep->bEndpointAddress & (LIBUSB_ENDPOINT_DIR_MASK)) {
                    uscpi->interrupt_ep = ep->bEndpointAddress;
                    sr_dbg("Interrupt EP %d", uscpi->interrupt_ep & 0x7f);
                }
            }
            found = 1;
        }
        libusb_free_config_descriptor(confdes);
        if (found)
            break;
    }

    if (!found) {
        sr_err("Failed to find USBTMC interface.");
        return SR_ERR;
    }

    if (libusb_kernel_driver_active(usb->devhdl, uscpi->interface) == 1) {
        if ((ret = libusb_detach_kernel_driver(usb->devhdl, uscpi->interface)) < 0) {
            sr_err("Failed to detach kernel driver: %s.", libusb_error_name(ret));
            return SR_ERR;
        }
        uscpi->detached_kernel_driver = 1;
    }
    int current_config = 0;
    if (libusb_get_configuration(usb->devhdl, &current_config) == 0 && current_config != config) {
        if ((ret = libusb_set_configuration(usb->devhdl, config)) < 0) {
            sr_err("Failed to set configuration: %s.", libusb_error_name(ret));
            return SR_ERR;
        }
    }

    if ((ret = libusb_claim_interface(usb->devhdl, uscpi->interface)) < 0) {
        sr_err("Failed to claim interface: %s.", libusb_error_name(ret));
        return SR_ERR;
    }

    /* Get capabilities. */
    ret = libusb_control_transfer(usb->devhdl, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, GET_CAPABILITIES, 0,
                                  uscpi->interface, capabilities, sizeof(capabilities), TRANSFER_TIMEOUT);
    if (ret == sizeof(capabilities)) {
        uscpi->usbtmc_int_cap = capabilities[4];
        uscpi->usbtmc_dev_cap = capabilities[5];
        uscpi->usb488_dev_cap = capabilities[15];
    }
    sr_dbg("Device capabilities: %s%s%s%s%s, %s, %s", uscpi->usb488_dev_cap & USB488_DEV_CAP_SCPI ? "SCPI, " : "",
           uscpi->usbtmc_dev_cap & USBTMC_DEV_CAP_TERMCHAR ? "TermChar, " : "",
           uscpi->usbtmc_int_cap & USBTMC_INT_CAP_LISTEN_ONLY ? "L3, " : uscpi->usbtmc_int_cap & USBTMC_INT_CAP_TALK_ONLY ? "" : "L4, ",
           uscpi->usbtmc_int_cap & USBTMC_INT_CAP_TALK_ONLY ? "T5, " : uscpi->usbtmc_int_cap & USBTMC_INT_CAP_LISTEN_ONLY ? "" : "T6, ",
           uscpi->usb488_dev_cap & USB488_DEV_CAP_SR1 ? "SR1" : "SR0", uscpi->usb488_dev_cap & USB488_DEV_CAP_RL1 ? "RL1" : "RL0",
           uscpi->usb488_dev_cap & USB488_DEV_CAP_DT1 ? "DT1" : "DT0");

    scpi_usbtmc_remote(uscpi);

    return SR_OK;
}

static void usbtmc_bulk_out_header_write(void *header, uint8_t MsgID, uint8_t bTag, uint32_t TransferSize, uint8_t bmTransferAttributes, char TermChar) {
    W8(header + 0, MsgID);
    W8(header + 1, bTag);
    W8(header + 2, ~bTag);
    W8(header + 3, 0);
    WL32(header + 4, TransferSize);
    W8(header + 8, bmTransferAttributes);
    W8(header + 9, TermChar);
    WL16(header + 10, 0);
}

static int usbtmc_bulk_in_header_read(void *header, uint8_t MsgID, unsigned char bTag, int32_t *TransferSize, uint8_t *bmTransferAttributes) {
    if (R8(header + 0) != MsgID || R8(header + 1) != bTag || R8(header + 2) != (unsigned char)~bTag)
        return SR_ERR;
    if (TransferSize)
        *TransferSize = RL32(header + 4);
    if (bmTransferAttributes)
        *bmTransferAttributes = R8(header + 8);

    return SR_OK;
}

static int scpi_usbtmc_bulkout(struct scpi_usbtmc_libusb *uscpi, uint8_t msg_id, const void *data, int32_t size, uint8_t transfer_attributes) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    int padded_size, ret, transferred;

    if (data && (size + USBTMC_BULK_HEADER_SIZE + 3) > (int)sizeof(uscpi->buffer)) {
        sr_err("USBTMC bulk out transfer is too big.");
        return SR_ERR;
    }

    uscpi->bTag++;
    uscpi->bTag += !uscpi->bTag; /* bTag == 0 is invalid so avoid it. */

    usbtmc_bulk_out_header_write(uscpi->buffer, msg_id, uscpi->bTag, size, transfer_attributes, 0);
    if (data)
        memcpy(uscpi->buffer + USBTMC_BULK_HEADER_SIZE, data, size);
    else
        size = 0;
    size += USBTMC_BULK_HEADER_SIZE;
    padded_size = (size + 3) & ~0x3;
    memset(uscpi->buffer + size, 0, padded_size - size);

    ret = libusb_bulk_transfer(usb->devhdl, uscpi->bulk_out_ep, uscpi->buffer, padded_size, &transferred, TRANSFER_TIMEOUT);
    if (ret < 0) {
        sr_err("USBTMC bulk out transfer error: %s.", libusb_error_name(ret));
        return SR_ERR;
    }

    if (transferred < padded_size) {
        sr_dbg("USBTMC bulk out partial transfer (%d/%d bytes).", transferred, padded_size);
        return SR_ERR;
    }

    return transferred - USBTMC_BULK_HEADER_SIZE;
}

static int scpi_usbtmc_bulkin_start(struct scpi_usbtmc_libusb *uscpi, uint8_t msg_id, unsigned char *data, int32_t size, uint8_t *transfer_attributes) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    int ret, transferred, message_size;

    ret = libusb_bulk_transfer(usb->devhdl, uscpi->bulk_in_ep, data, size, &transferred, TRANSFER_TIMEOUT);
    if (ret < 0) {
        sr_err("USBTMC bulk in transfer error: %s.", libusb_error_name(ret));
        return SR_ERR;
    }

    if (usbtmc_bulk_in_header_read(data, msg_id, uscpi->bTag, &message_size, transfer_attributes) != SR_OK) {
        sr_err("USBTMC invalid bulk in header.");
        return SR_ERR;
    }

    message_size += USBTMC_BULK_HEADER_SIZE;
    uscpi->response_length = MIN(transferred, message_size);
    uscpi->response_bytes_read = USBTMC_BULK_HEADER_SIZE;
    uscpi->remaining_length = message_size - uscpi->response_length;

    return transferred - USBTMC_BULK_HEADER_SIZE;
}

static int scpi_usbtmc_bulkin_continue(struct scpi_usbtmc_libusb *uscpi, unsigned char *data, int size) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    int ret, transferred;

    ret = libusb_bulk_transfer(usb->devhdl, uscpi->bulk_in_ep, data, size, &transferred, TRANSFER_TIMEOUT);
    if (ret < 0) {
        sr_err("USBTMC bulk in transfer error: %s.", libusb_error_name(ret));
        return SR_ERR;
    }

    uscpi->response_length = MIN(transferred, uscpi->remaining_length);
    uscpi->response_bytes_read = 0;
    uscpi->remaining_length -= uscpi->response_length;

    return transferred;
}

int scpi_usbtmc_libusb_send(struct scpi_usbtmc_libusb *uscpi, const QByteArray &command) {

    if (scpi_usbtmc_bulkout(uscpi, DEV_DEP_MSG_OUT, command.data(), command.length(), EOM) <= 0){
        qDebug() << "scpi_usbtmc_bulkout@scpi_usbtmc_libusb_send failed." ;
        return SR_ERR;
    }

    sr_spew("Successfully sent SCPI command: '%s'.", command.data());

    return SR_OK;
}

int scpi_usbtmc_libusb_read_begin(struct scpi_usbtmc_libusb *uscpi) {

    uscpi->remaining_length = 0;

    if (scpi_usbtmc_bulkout(uscpi, REQUEST_DEV_DEP_MSG_IN, NULL, INT32_MAX, 0) < 0){
        qDebug() << "scpi_usbtmc_bulkout@scpi_usbtmc_libusb_read_begin failed." ;
        return SR_ERR;
    }
    if (scpi_usbtmc_bulkin_start(uscpi, DEV_DEP_MSG_IN, uscpi->buffer, sizeof(uscpi->buffer), &uscpi->bulkin_attributes) < 0){
        qDebug() << "scpi_usbtmc_bulkin_start@scpi_usbtmc_libusb_read_begin failed." ;
        return SR_ERR;
    }

    return SR_OK;
}

int scpi_usbtmc_libusb_read_data(struct scpi_usbtmc_libusb *uscpi, char *buf, int maxlen) {
    int read_length;

    if (uscpi->response_bytes_read >= uscpi->response_length) {
        if (uscpi->remaining_length > 0) {
            if (scpi_usbtmc_bulkin_continue(uscpi, uscpi->buffer, sizeof(uscpi->buffer)) <= 0)
                return SR_ERR;
        } else {
            if (uscpi->bulkin_attributes & EOM)
                return SR_ERR;
            if (scpi_usbtmc_libusb_read_begin(uscpi) < 0)
                return SR_ERR;
        }
    }

    read_length = MIN(uscpi->response_length - uscpi->response_bytes_read, maxlen);

    memcpy(buf, uscpi->buffer + uscpi->response_bytes_read, read_length);

    uscpi->response_bytes_read += read_length;

    return read_length;
}

int scpi_usbtmc_libusb_read_complete(struct scpi_usbtmc_libusb *uscpi) {
    return uscpi->response_bytes_read >= uscpi->response_length && uscpi->remaining_length <= 0 && uscpi->bulkin_attributes & EOM;
}

int scpi_usbtmc_libusb_close(struct scpi_usbtmc_libusb *uscpi) {
    struct sr_usb_dev_inst *usb = &uscpi->usb;
    int ret;

    if (!usb->devhdl)
        return SR_ERR;

    scpi_usbtmc_local(uscpi);

    if ((ret = libusb_release_interface(usb->devhdl, uscpi->interface)) < 0)
        sr_err("Failed to release interface: %s.", libusb_error_name(ret));

    if (uscpi->detached_kernel_driver) {
        if ((ret = libusb_attach_kernel_driver(usb->devhdl, uscpi->interface)) < 0)
            sr_err("Failed to re-attach kernel driver: %s.", libusb_error_name(ret));

        uscpi->detached_kernel_driver = 0;
    }
    sr_usb_close(usb);

    return SR_OK;
}




#endif
