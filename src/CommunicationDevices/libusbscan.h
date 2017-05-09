#ifndef LIBUSBSCAN_H
#define LIBUSBSCAN_H

#include <QStringList>

class LIBUSBScan {
    public:
    LIBUSBScan();
    ~LIBUSBScan();

    static QStringList scan();
};

#endif // LIBUSBSCAN_H
