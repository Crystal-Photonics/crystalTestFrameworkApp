#include "dummydatacreator.h"
#include "ui_dummydatacreator.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <fstream>

DummyDataCreator::DummyDataCreator(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DummyDataCreator) {
    ui->setupUi(this);
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Data Engine Input files (*.json)"));
    if (dialog.exec()) {
        std::ifstream f(dialog.selectedFiles()[0].toStdString());
        data_engine.set_source(f);

        instance_names = data_engine.get_instance_count_names();
        uint i = 0;
        for (auto name : instance_names) {
            QSpinBox *sb = new QSpinBox(ui->groupBox);
            QLabel *label = new QLabel(ui->groupBox);
            spinboxes.append(sb);
            sb->setValue(2);
            label->setText(name + ":");
            ui->gridLayout_2->addWidget(label, i, 0);
            ui->gridLayout_2->addWidget(sb, i, 1);
            i++;
        }

        template_config_filename = dialog.selectedFiles()[0] + ".template_conf";
        load_gui_from_json();
    }
}

DummyDataCreator::~DummyDataCreator() {
    delete ui;
}

void DummyDataCreator::on_pushButton_clicked() {
    for (int i = 0; i < instance_names.count(); i++) {
        data_engine.set_instance_count(instance_names[i], spinboxes[i]->value());
    }
    data_engine.fill_engine_with_dummy_data();
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Report Template (*.lrxml)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec()) {
        const auto db_name = dialog.selectedFiles()[0] + ".db";
        if (QFile::exists(db_name)) {
            auto result = QMessageBox::question(this, QString("File already exists"),
                                                QString("The file %1 already exists. Do you want to overwrite this file?").arg(db_name));
            if (result == QMessageBox::No) {
                return;
            }
        }
        save_gui_to_json();
        const auto report_title = ui->edt_title->text();
        const auto image_footer_path = ui->edt_image_footer->text();
        const auto image_header_path = ui->edt_image_header->text();
        QList<PrintOrderItem> print_order;
        for (int i = 0; i < ui->section_list->topLevelItemCount(); i++) {
            PrintOrderItem item{};
            QTreeWidgetItem *tree_item = ui->section_list->topLevelItem(i);
            item.print_enabled = tree_item->checkState(0) == Qt::Checked;
            item.section_name = tree_item->text(1);
            item.print_as_text_field = tree_item->checkState(2) == Qt::Checked;
            print_order.append(item);
        }

        data_engine.generate_template(dialog.selectedFiles()[0], db_name, report_title, image_footer_path, image_header_path, print_order);
        { //TODO: put into data_engine

            auto db = QSqlDatabase::addDatabase("QSQLITE");
            QFile::remove(db_name);
            db.setDatabaseName(db_name);
            db.open();
            data_engine.fill_database(db);
        }
    }
    close();
}

void DummyDataCreator::on_pushButton_2_clicked() {
    close();
}

void DummyDataCreator::load_gui_from_json() {
    QFile loadFile(template_config_filename);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject obj = loadDoc.object();

    QString report_title = obj["report_title"].toString().trimmed();
    ui->edt_title->setText(report_title);
    ui->edt_image_footer->setText(obj["image_footer"].toString().trimmed());
    ui->edt_image_header->setText(obj["image_header"].toString().trimmed());
    ui->section_list->clear();
    QJsonArray section_order = obj["section_order"].toArray();
    auto sections = data_engine.get_section_names();
    for (auto jitem : section_order) {
        QJsonObject obj = jitem.toObject();
        auto sn = obj["name"].toString();
        if (sections.contains(sn)) {
            sections.removeAll(sn);
            QStringList sl;
            sl.append("");
            sl.append(sn);
            sl.append("");
            sl.append("");
            QTreeWidgetItem *item = new QTreeWidgetItem(sl);
            item->setFlags(Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            if (obj["print"].toBool()) {
                item->setCheckState(0, Qt::Checked);
            } else {
                item->setCheckState(0, Qt::Unchecked);
            }

            if (data_engine.section_uses_variants(sn)) {
                //  item->setCheckState(2, Qt::);
            } else {
                if (obj["as_text_field"].toBool()) {
                    item->setCheckState(2, Qt::Checked);
                } else {
                    item->setCheckState(2, Qt::Unchecked);
                }
            }

            ui->section_list->addTopLevelItem(item);
        }
    }
    for (auto sn : sections) {
        QStringList sl;
        sl.append("");
        sl.append(sn);
        sl.append("");
        sl.append("");
        QTreeWidgetItem *item = new QTreeWidgetItem(sl);
        item->setCheckState(0, Qt::Checked);
        if (data_engine.section_uses_variants(sn)) {
            //  item->setCheckState(2, Qt::);
        } else {
            item->setCheckState(2, Qt::Unchecked);
        }
        item->setFlags(Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        ui->section_list->addTopLevelItem(item);
    }
}

void DummyDataCreator::save_gui_to_json() {
    if (template_config_filename == "") {
        return;
    }
    QFile saveFile(template_config_filename);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return;
    }
    QJsonObject obj;
    obj["report_title"] = ui->edt_title->text();
    obj["image_footer"] = ui->edt_image_footer->text();
    obj["image_header"] = ui->edt_image_header->text();

    QJsonArray section_order;
    for (int i = 0; i < ui->section_list->topLevelItemCount(); i++) {
        QJsonObject obj;
        QTreeWidgetItem *item = ui->section_list->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            obj["print"] = true;
        } else {
            obj["print"] = false;
        }

        obj["name"] = item->text(1);
        if (item->checkState(2) == Qt::Checked) {
            obj["as_text_field"] = true;
        } else {
            obj["as_text_field"] = false;
        }
        section_order.append(obj);
    }
    obj["section_order"] = section_order;
    QJsonDocument saveDoc(obj);
    saveFile.write(saveDoc.toJson());
}

void DummyDataCreator::on_btn_image_header_clicked() {
    auto fileName = QFileDialog::getOpenFileName(this, tr("Open Header Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));
    ui->edt_image_header->setText(fileName);
}

void DummyDataCreator::on_btn_image_footer_clicked() {
    auto fileName = QFileDialog::getOpenFileName(this, tr("Open Footer Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));
    ui->edt_image_footer->setText(fileName);
}
