#include "devicematcher.h"
#include "ui_devicematcher.h"

DeviceMatcher::DeviceMatcher(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceMatcher)
{
    ui->setupUi(this);
}

DeviceMatcher::~DeviceMatcher()
{
    delete ui;
}
