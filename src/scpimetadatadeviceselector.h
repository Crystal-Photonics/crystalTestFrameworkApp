#ifndef SCPIMETADATADEVICESELECTOR_H
#define SCPIMETADATADEVICESELECTOR_H

#include <QDialog>
#include "scpimetadata.h"
#include <QTreeWidgetItem>

namespace Ui {
class SCPIMetaDataDeviceSelector;
}

class SCPIMetaDataDeviceSelector : public QDialog
{
    Q_OBJECT

public:
    explicit SCPIMetaDataDeviceSelector(QWidget *parent = 0);

    ~SCPIMetaDataDeviceSelector();
    void load_devices(QString port_name, SCPIDeviceType scpi_meta_data);
    SCPIDeviceType get_device();

private slots:
    void on_treeWidget_itemChanged(QTreeWidgetItem *item, int column);
    void on_btn_cancel_clicked();
    void on_btn_ok_clicked();

private:
    SCPIDeviceType scpi_meta_data;
    QString port_name;
    Ui::SCPIMetaDataDeviceSelector *ui;
    void make_gui();
    void calc_ok_enabled();
    int selected_device_detail_index = -1;
    void align_columns();
};

#endif // SCPIMETADATADEVICESELECTOR_H
