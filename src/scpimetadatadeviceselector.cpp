#include "scpimetadatadeviceselector.h"
#include "ui_scpimetadatadeviceselector.h"

SCPIMetaDataDeviceSelector::SCPIMetaDataDeviceSelector(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SCPIMetaDataDeviceSelector)
{
    ui->setupUi(this);
}

SCPIMetaDataDeviceSelector::~SCPIMetaDataDeviceSelector()
{
    delete ui;
}

void SCPIMetaDataDeviceSelector::on_buttonBox_accepted()
{

}
