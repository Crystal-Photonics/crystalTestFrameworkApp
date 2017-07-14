#include "scpimetadatadeviceselector.h"
#include "ui_scpimetadatadeviceselector.h"

SCPIMetaDataDeviceSelector::SCPIMetaDataDeviceSelector(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SCPIMetaDataDeviceSelector) {
    ui->setupUi(this);
}

SCPIMetaDataDeviceSelector::~SCPIMetaDataDeviceSelector() {
    delete ui;
}

void SCPIMetaDataDeviceSelector::load_devices(QString port_name, DeviceMetaDataGroup scpi_meta_data) {
    this->port_name = port_name;
    this->scpi_meta_data = scpi_meta_data;
    make_gui();
}

DeviceMetaDataGroup SCPIMetaDataDeviceSelector::get_device() {
    DeviceMetaDataGroup result = scpi_meta_data;
    result.devices.clear();
    if ((-1 < selected_device_detail_index) && (selected_device_detail_index < scpi_meta_data.devices.count())) {
        result.devices.append(scpi_meta_data.devices[selected_device_detail_index]);
    }
    return result;
}


void SCPIMetaDataDeviceSelector::align_columns() {
    ui->treeWidget->expandAll();
    for (int i = 0; i < ui->treeWidget->columnCount(); i++) {
        ui->treeWidget->resizeColumnToContents(i);
    }
}

void SCPIMetaDataDeviceSelector::make_gui() {
    QTreeWidgetItem *wi = new QTreeWidgetItem(ui->treeWidget);
    wi->setText(0, scpi_meta_data.commondata.device_name + "@" + port_name);
    for (auto &d : scpi_meta_data.devices) {
        QTreeWidgetItem *di = new QTreeWidgetItem(wi);
        di->setText(0, d.serial_number);
        di->setText(1, d.get_approved_state_str());
        di->setText(2, d.note);
        di->setCheckState(0, Qt::Unchecked);
    }
    ui->treeWidget->insertTopLevelItem(0, wi);
    calc_ok_enabled();
    align_columns();
}

void SCPIMetaDataDeviceSelector::calc_ok_enabled() {
    int checked_items = 0;
    selected_device_detail_index = -1;
    auto top_level_widget = ui->treeWidget->topLevelItem(0);
    for (int i = 0; i < top_level_widget->childCount(); i++) {
        auto item_to_uncheck = top_level_widget->child(i);

        if (item_to_uncheck->checkState(0) == Qt::Checked) {
            checked_items++;
            selected_device_detail_index = i;
        }
    }
    ui->btn_ok->setEnabled(checked_items == 1);
}

void SCPIMetaDataDeviceSelector::on_treeWidget_itemChanged(QTreeWidgetItem *item, int column) {
    (void)column;
    if (item->checkState(0) == Qt::Checked) {
        auto top_level_widget = ui->treeWidget->topLevelItem(0);
        for (int i = 0; i < top_level_widget->childCount(); i++) {
            auto item_to_uncheck = top_level_widget->child(i);
            if (item == item_to_uncheck) {
                continue;
            }
            item_to_uncheck->setCheckState(0, Qt::Unchecked);
        }
    }
    calc_ok_enabled();
}

void SCPIMetaDataDeviceSelector::on_btn_cancel_clicked() {
    selected_device_detail_index = -1;
    close();
}

void SCPIMetaDataDeviceSelector::on_btn_ok_clicked() {
    close();
}
