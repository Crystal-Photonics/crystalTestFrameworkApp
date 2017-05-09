#include "libusbscan.h"

#include "CommunicationDevices/usbtmc_libusb.h"
#include "libusb-1.0/libusb.h"

#include <QDebug>

LIBUSBScan::LIBUSBScan()
{
    int r = libusb_init(nullptr);
    if (r < 0) {
        qDebug() << QString("could not initialize lib usb. (%1)").arg(QString::number(r));
    }
}

QStringList LIBUSBScan::scan() {
    return list_usb_tmc_devices();
}


LIBUSBScan::~LIBUSBScan() {
    libusb_exit(nullptr);
}
