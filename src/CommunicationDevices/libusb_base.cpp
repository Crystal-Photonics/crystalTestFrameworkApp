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

#include "libusb_base.h"

#include <assert.h>
#include <config.h>
#include <memory.h>
#include <stdlib.h>

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QRegExp>
#include <QString>
#include <QStringList>

#if !HAVE_LIBUSB_OS_HANDLE
typedef int libusb_os_handle;
#endif
static int cur_loglevel = SR_LOG_SPEW; /* Show errors+warnings per default. */

#define LOGLEVEL_TIMESTAMP SR_LOG_DBG

static int sr_logv(int loglevel, const char *format, va_list args) {
    static auto sr_log_start_time = QDateTime::currentDateTime().toMSecsSinceEpoch();
    /* This specific log callback doesn't need the void pointer data. */

    /* Only output messages of at least the selected loglevel(s). */
    if (loglevel > cur_loglevel) {
        return SR_OK;
    }

    if (cur_loglevel >= LOGLEVEL_TIMESTAMP) {
        auto elapsed_ms = QDateTime::currentDateTime().toMSecsSinceEpoch() - sr_log_start_time;

        QString datetime_str = QDateTime::fromMSecsSinceEpoch(elapsed_ms).toString("hh:mm:ss:zzz");
        qDebug() << datetime_str;
    } else {
    }
    QString stringding = QString::vasprintf(format, args);

    stringding = stringding.replace("\n", " ");
    qDebug() << stringding;

    return SR_OK;
}

int sr_log(int loglevel, const char *format, ...) {
    int ret;
    va_list args;

    va_start(args, format);
    /// qDebug()
    ret = sr_logv(loglevel, format, args);
    va_end(args);

    return ret;
}

/**
 * Allocate and init a struct for a USB device instance.
 *
 * @param[in] bus @copydoc sr_usb_dev_inst::bus
 * @param[in] address @copydoc sr_usb_dev_inst::address
 * @param[in] hdl @copydoc sr_usb_dev_inst::devhdl
 *
 * @return The struct sr_usb_dev_inst * for USB device instance.
 *
 * @private
 */
struct sr_usb_dev_inst sr_usb_dev_inst_new(uint8_t bus, uint8_t address, struct libusb_device_handle *hdl) {
    struct sr_usb_dev_inst udi;

    //udi = g_malloc0(sizeof(struct sr_usb_dev_inst));
    udi.bus = bus;
    udi.address = address;
    udi.devhdl = hdl;

    return udi;
}

/**
 * Free struct sr_usb_dev_inst * allocated by sr_usb_dev_inst().
 *
 * @param usb The struct sr_usb_dev_inst * to free. If NULL, this
 *            function does nothing.
 *
 * @private
 */
void sr_usb_dev_inst_free(struct sr_usb_dev_inst *usb) {
    //;
}



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
QList<sr_usb_dev_inst> sr_usb_find(libusb_context *usb_ctx, QString conn) {
    struct libusb_device **devlist;
    struct libusb_device_descriptor des;
    QList<sr_usb_dev_inst> devices{};
    int vid, pid, bus, addr, b, a, ret, i;


    vid = pid = bus = addr = 0;
    QStringList items = conn.split("/");
    QStringList regex_matches = items.last().split(":");

    if (regex_matches.count() == 2) {
        bool ok;
        vid = regex_matches[0].toInt(&ok, 16);
        pid = regex_matches[2].toInt(&ok, 16);

        sr_dbg("Trying to find USB device with VID:PID = %04x:%04x.", vid, pid);
    } else {
        bool ok;
        QStringList regex_matches = items.last().split(".");
        bus = regex_matches[0].toInt(&ok, 10);
        addr = regex_matches[1].toInt(&ok, 10);

        sr_dbg(
            "Trying to find USB device with bus.address = "
            "%d.%d.",
            bus, addr);
    }

    if (vid + pid + bus + addr == 0) {
        sr_err("Neither VID:PID nor bus.address was specified.");
        return {};
    }

    if (bus > 255) {
        sr_err("Invalid bus specified: %d.", bus);
        return {};
    }

    if (addr > 127) {
        sr_err("Invalid address specified: %d.", addr);
        return {};
    }

    /* Looks like a valid USB device specification, but is it connected? */

    libusb_get_device_list(usb_ctx, &devlist);
    for (i = 0; devlist[i]; i++) {
        if ((ret = libusb_get_device_descriptor(devlist[i], &des))) {
            sr_err("Failed to get device descriptor: %s.", libusb_error_name(ret));
            continue;
        }

        if (vid + pid && (des.idVendor != vid || des.idProduct != pid))
            continue;

        b = libusb_get_bus_number(devlist[i]);
        a = libusb_get_device_address(devlist[i]);
        if (bus + addr && (b != bus || a != addr))
            continue;

        sr_dbg(
            "Found USB device (VID:PID = %04x:%04x, bus.address = "
            "%d.%d).",
            des.idVendor, des.idProduct, b, a);

        devices.append(sr_usb_dev_inst_new(libusb_get_bus_number(devlist[i]), libusb_get_device_address(devlist[i]), NULL));
    }
    libusb_free_device_list(devlist, 1);

    sr_dbg("Found %d device(s).", devices.count());

    return devices;
}

int sr_usb_open(libusb_context *usb_ctx, struct sr_usb_dev_inst *usb) {
    struct libusb_device **devlist;

    int ret = 0;

    sr_dbg("Trying to open USB device %d.%d.", usb->bus, usb->address);

    int cnt = libusb_get_device_list(usb_ctx, &devlist);
    if (cnt < 0) {
        sr_err("Failed to retrieve device list: %s.", libusb_error_name(cnt));
        return SR_ERR;
    }

    ret = SR_ERR;
    for (int i = 0; i < cnt; i++) {
        struct libusb_device_descriptor des;
        int r  = libusb_get_device_descriptor(devlist[i], &des);
        if (r < 0) {
            sr_err("Failed to get device descriptor: %s.", libusb_error_name(r));
            continue;
        }

        int b = libusb_get_bus_number(devlist[i]);
        int a = libusb_get_device_address(devlist[i]);
        if (b != usb->bus || a != usb->address)
            continue;

        if ((r = libusb_open(devlist[i], &usb->devhdl)) < 0) {
            sr_err("Failed to open device: %s.", libusb_error_name(r));
            break;
        }

        sr_dbg(
            "Opened USB device (VID:PID = %04x:%04x, bus.address = "
            "%d.%d).",
            des.idVendor, des.idProduct, b, a);

        ret = SR_OK;
        break;
    }

    libusb_free_device_list(devlist, 1);

    return ret;
}

#if 1
void sr_usb_close(struct sr_usb_dev_inst *usb) {
    libusb_close(usb->devhdl);
    usb->devhdl = NULL;
    sr_dbg("Closed USB device %d.%d.", usb->bus, usb->address);
}
#endif
