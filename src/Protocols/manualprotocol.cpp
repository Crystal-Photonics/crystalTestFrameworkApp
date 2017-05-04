#include "manualprotocol.h"
#include <QTreeWidgetItem>

ManualProtocol::ManualProtocol()
    : Protocol{"manual"} {}

ManualProtocol::~ManualProtocol() {}

bool ManualProtocol::is_correct_protocol() {
    return true;
}

void ManualProtocol::set_ui_description(QTreeWidgetItem *ui_entry) {
    ui_entry->setText(1, "Manual");
    ui_entry->setText(2, meta_data.commondata.device_name);

    ui_entry->setToolTip(0, get_summary_lcl());
    ui_entry->addChild(new QTreeWidgetItem(ui_entry, QStringList{} << get_summary_lcl()));
}

DeviceMetaDataApprovedState ManualProtocol::get_approved_state() {
    if (meta_data.devices.count()) {
        return meta_data.devices[0].get_approved_state();
    } else {
        return DeviceMetaDataApprovedState::Unknown;
    }
}


QString ManualProtocol::get_approved_state_str() {
    if (meta_data.devices.count()) {
        return meta_data.devices[0].get_approved_state_str();
    } else {
        return "unknown";
    }
}

void ManualProtocol::set_meta_data(DeviceMetaDataGroup meta_data) {
    this->meta_data = meta_data;
}

std::string ManualProtocol::get_name() {
    return meta_data.commondata.device_name.toStdString();
}

std::string ManualProtocol::get_manufacturer() {
    return meta_data.commondata.manufacturer.toStdString();
}

std::string ManualProtocol::get_description() {
    return meta_data.commondata.description.toStdString();
}

std::string ManualProtocol::get_serial_number() {
    if (meta_data.devices.count()) {
        return meta_data.devices[0].serial_number.toStdString();
    } else {
        return "";
    }
}

std::string ManualProtocol::get_notes() {
    if (meta_data.devices.count()) {
        return meta_data.devices[0].note.toStdString();
    } else {
        return "";
    }
}

std::string ManualProtocol::get_summary() {
    return get_summary_lcl().toStdString();
}

QString ManualProtocol::get_summary_lcl() {
    QStringList statustip;
    statustip.append("Name: " + meta_data.commondata.device_name);
    statustip.append("Manufacturer: " + meta_data.commondata.manufacturer);
    statustip.append("Description: " + meta_data.commondata.description);
    if (meta_data.devices.count()) {
        statustip.append("Serial-Number: " + meta_data.devices[0].serial_number);
        statustip.append("Calibration: " + meta_data.devices[0].get_approved_state_str());
        statustip.append("Notes: " + meta_data.devices[0].note);
    }
    return statustip.join("\n");
}
