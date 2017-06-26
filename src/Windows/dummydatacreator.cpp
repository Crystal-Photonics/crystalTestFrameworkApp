#include "dummydatacreator.h"
#include "ui_dummydatacreator.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QSqlDatabase>
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
        data_engine.generate_template(dialog.selectedFiles()[0], db_name, ui->edt_title->text());
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
    QJsonDocument saveDoc(obj);
    saveFile.write(saveDoc.toJson());
}
