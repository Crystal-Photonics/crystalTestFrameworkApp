/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2012 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2012 Bert Vermeulen <bert@biot.com>
 * Copyright (C) 2015 Daniel Elstner <daniel.kitta@gmail.com>
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

#ifndef libusb_base_h
#define libusb_base_h
#include "libusb-1.0/libusb.h"

#include <QList>
#include <inttypes.h>




/** USB device instance */
struct sr_usb_dev_inst {
    /** USB bus */
    uint8_t bus;
    /** Device address on USB bus */
    uint8_t address;
    /** libusb device handle */
    struct libusb_device_handle *devhdl;
};





/** libsigrok loglevels. */
enum sr_loglevel {
    SR_LOG_NONE = 0, /**< Output no messages at all. */
    SR_LOG_ERR  = 1, /**< Output error messages. */
    SR_LOG_WARN = 2, /**< Output warnings. */
    SR_LOG_INFO = 3, /**< Output informational messages. */
    SR_LOG_DBG  = 4, /**< Output debug messages. */
    SR_LOG_SPEW = 5, /**< Output very noisy debug messages. */
};

/** Status/error codes returned by libsigrok functions. */
enum sr_error_code {
    SR_OK                =  0, /**< No error. */
    SR_ERR               = -1, /**< Generic/unspecified error. */
    SR_ERR_MALLOC        = -2, /**< Malloc/calloc/realloc error. */
    SR_ERR_ARG           = -3, /**< Function argument error. */
    SR_ERR_BUG           = -4, /**< Errors hinting at internal bugs. */
    SR_ERR_SAMPLERATE    = -5, /**< Incorrect samplerate. */
    SR_ERR_NA            = -6, /**< Not applicable. */
    SR_ERR_DEV_CLOSED    = -7, /**< Device is closed, but must be open. */
    SR_ERR_TIMEOUT       = -8, /**< A timeout occurred. */
    SR_ERR_CHANNEL_GROUP = -9, /**< A channel group must be specified. */
    SR_ERR_DATA          =-10, /**< Data is invalid.  */
    SR_ERR_IO            =-11, /**< Input/output error. */

    /* Update sr_strerror()/sr_strerror_name() (error.c) upon changes! */
};

/** Type definition for callback function for data reception. */
typedef int (*sr_receive_data_callback)(int fd, int revents, void *cb_data);

#define LOG_PREFIX "scpi_usbtmc"

int sr_log(int loglevel, const char *format, ...);

/* Message logging helpers with subsystem-specific prefix string. */
#define sr_spew(...)	sr_log(SR_LOG_SPEW, LOG_PREFIX ": " __VA_ARGS__)
#define sr_dbg(...)     sr_log(SR_LOG_DBG,  LOG_PREFIX ": " __VA_ARGS__)
#define sr_info(...)	sr_log(SR_LOG_INFO, LOG_PREFIX ": " __VA_ARGS__)
#define sr_warn(...)	sr_log(SR_LOG_WARN, LOG_PREFIX ": " __VA_ARGS__)
#define sr_err(...)     sr_log(SR_LOG_ERR,  LOG_PREFIX ": " __VA_ARGS__)



/**
 * Find USB devices according to a connection string.
 *
 * @param usb_ctx libusb context to use while scanning.
 * @param conn Connection string specifying the device(s) to match. This
 * can be of the form "<bus>.<address>", or "<vendorid>.<productid>".
 *
 * @return A GSList of struct sr_usb_dev_inst, with bus and address fields
 * matching the device that matched the connection string. The GSList and
 * its contents must be freed by the caller.
 */


QList<sr_usb_dev_inst> sr_usb_find(libusb_context *usb_ctx, QString conn);


int sr_usb_open(libusb_context *usb_ctx, struct sr_usb_dev_inst *usb);

void sr_usb_close(struct sr_usb_dev_inst *usb);

int usb_source_add(struct sr_session *session, struct sr_context *ctx,
		int timeout, sr_receive_data_callback cb, void *cb_data);


int usb_source_remove(struct sr_session *session, struct sr_context *ctx);
int usb_get_port_path(libusb_device *dev, char *path, int path_len);

#endif
