#ifndef DEVICEMATCHER_H
#define DEVICEMATCHER_H

#include <QDialog>

namespace Ui {
class DeviceMatcher;
}

class DeviceMatcher : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceMatcher(QWidget *parent = 0);
    ~DeviceMatcher();

private:
    Ui::DeviceMatcher *ui;
};

#endif // DEVICEMATCHER_H
