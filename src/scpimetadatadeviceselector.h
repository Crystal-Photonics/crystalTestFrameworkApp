#ifndef SCPIMETADATADEVICESELECTOR_H
#define SCPIMETADATADEVICESELECTOR_H

#include <QDialog>

namespace Ui {
class SCPIMetaDataDeviceSelector;
}

class SCPIMetaDataDeviceSelector : public QDialog
{
    Q_OBJECT

public:
    explicit SCPIMetaDataDeviceSelector(QWidget *parent = 0);
    ~SCPIMetaDataDeviceSelector();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::SCPIMetaDataDeviceSelector *ui;
};

#endif // SCPIMETADATADEVICESELECTOR_H
