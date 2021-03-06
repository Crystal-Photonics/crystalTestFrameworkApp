#include "dummydatacreator.h"
#include "ui_dummydatacreator.h"
#include <QComboBox>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <fstream>

const static QStringList textfield_places{"Report Header", "Page Header", "Page Footer", "Report Footer"};

static TextFieldDataBandPlace str_to_textfield_place_enum(QString str) {
    if (str == textfield_places[0]) {
        return TextFieldDataBandPlace::report_header;
    } else if (str == textfield_places[1]) {
        return TextFieldDataBandPlace::page_header;
    } else if (str == textfield_places[2]) {
        return TextFieldDataBandPlace::page_footer;
    } else if (str == textfield_places[3]) {
        return TextFieldDataBandPlace::report_footer;
    } else if (str == "") {
        return TextFieldDataBandPlace::none;
    } else {
        assert(0);
    }
    return TextFieldDataBandPlace::none;
}

QWidget *SpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const {
    QSpinBox *editor = new QSpinBox(parent);
    editor->setFrame(false);
    editor->setMinimum(1);
    editor->setMaximum(10);
    return editor;
}

void SpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    int value = index.model()->data(index, Qt::EditRole).toInt();
    QSpinBox *spinBox = static_cast<QSpinBox *>(editor);
    spinBox->setValue(value);
}

void SpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /* index */) const {
    editor->setGeometry(option.rect);
}

void SpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QSpinBox *spinBox = static_cast<QSpinBox *>(editor);
    spinBox->interpretText();
    int value = spinBox->value();
    model->setData(index, value, Qt::EditRole);
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const {
    QComboBox *editor = new QComboBox(parent);
    editor->setFrame(false);
    editor->addItems(my_items);
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    auto value = index.model()->data(index, Qt::EditRole).toString();
    QComboBox *comboBox = static_cast<QComboBox *>(editor);
    comboBox->setCurrentText(value);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /* index */) const {
    editor->setGeometry(option.rect);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QComboBox *comboBox = static_cast<QComboBox *>(editor);
    auto value = comboBox->currentText();
    model->setData(index, value, Qt::EditRole);
}

DummyDataCreator::DummyDataCreator(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DummyDataCreator) {
    ui->setupUi(this);
    QFileDialog dialog(this);
    is_valid_data_engine = true;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Data Engine Input files (*.json)"));
    if (dialog.exec()) {
        std::ifstream f(dialog.selectedFiles()[0].toStdString());
        try {
            data_engine.set_source(f);

            instance_names = data_engine.get_instance_count_names();
            uint i = 0;
            for (auto name : instance_names) {
                QSpinBox *sb = new QSpinBox(ui->groupBox);
                QLabel *label = new QLabel(ui->groupBox);
                sb->setMaximum(1000);
                spinboxes.append(sb);
                sb->setValue(2);
                label->setText(name + ":");
                ui->gridLayout_2->addWidget(label, i, 0);
                ui->gridLayout_2->addWidget(sb, i, 1);
                i++;
            }

            template_config_filename = dialog.selectedFiles()[0] + ".template_conf";
            load_gui_from_json();
        } catch (DataEngineError &e) {
            QMessageBox::warning(this, QString("Dataengine error"), QString("Dataengine error:\n\n %1").arg(e.what()));
            is_valid_data_engine = false;
        }
    }
}

DummyDataCreator::~DummyDataCreator() {
    delete ui;
}

void DummyDataCreator::on_pushButton_clicked() {
    bool close_dialog = true;
    try {
        for (int i = 0; i < instance_names.count(); i++) {
            data_engine.set_instance_count(instance_names[i], spinboxes[i]->value());
        }
        data_engine.fill_engine_with_dummy_data();
    } catch (DataEngineError &e) {
        QMessageBox::warning(this, QString("Dataengine error"), QString("Dataengine error:\n\n %1").arg(e.what()));
        close_dialog = false;
        return;
    }

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Report Template (*.lrxml)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec()) {
        const auto db_name = dialog.selectedFiles()[0] + ".db";
        save_gui_to_json();
        const auto report_title = ui->edt_title->text();
        const auto image_footer_path = ui->edt_image_footer->text();
        const auto image_header_path = ui->edt_image_header->text();
        const auto approved_by_field_id = ui->edt_approved->text();
        QList<PrintOrderItem> print_order;
        for (int i = 0; i < ui->section_list->topLevelItemCount(); i++) {
            PrintOrderItem item{};
            QTreeWidgetItem *tree_item = ui->section_list->topLevelItem(i);
            item.print_enabled = tree_item->checkState(0) == Qt::Checked;
            item.section_name = tree_item->text(1);
            item.print_as_text_field = tree_item->checkState(2) == Qt::Checked;

            bool ok = false;
            auto col_count = tree_item->text(3).toInt(&ok);
            if (!ok) {
                col_count = 2;
            }
            item.text_field_column_count = col_count;
            item.text_field_place = str_to_textfield_place_enum(tree_item->text(4));
            print_order.append(item);
        }

        QString static_text_report_header = ui->text_report_header->toPlainText();
        QString static_text_page_header = ui->text_page_header->toPlainText();

        QString static_text_page_footer = ui->text_page_footer->toPlainText();
        QString static_text_report_footer_above_signature = ui->text_report_footer_above->toPlainText();

        QString static_text_report_footer_beneath_signature = ui->text_report_footer_beneath->toPlainText();

        data_engine.generate_template(dialog.selectedFiles()[0], db_name, report_title, image_footer_path, image_header_path, approved_by_field_id,
                                      static_text_report_header, static_text_page_header, static_text_page_footer, static_text_report_footer_above_signature,
                                      static_text_report_footer_beneath_signature,

                                      print_order);
        { //TODO: put into data_engine

            QFile::remove(db_name);
            QSqlDatabase db;
            if (QSqlDatabase::contains()) {
                db = QSqlDatabase::database();
            } else {
                db = QSqlDatabase::addDatabase("QSQLITE");
            }

            db.setDatabaseName(db_name);
            db.open();
            try {
                data_engine.fill_database(db);
            } catch (DataEngineError &e) {
                QMessageBox::warning(this, QString("Dataengine error"), QString("Dataengine error:\n\n %1").arg(e.what()));
                close_dialog = false;
            }
            db.close();
        }
    }
    if (close_dialog) {
        close();
    }
}

void DummyDataCreator::on_pushButton_2_clicked() {
    close();
}

void DummyDataCreator::load_gui_from_json() {
    const Qt::ItemFlags item_flags_editable = Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    const Qt::ItemFlags item_flags_readonly = Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    auto sections = data_engine.get_section_names();
    QFile loadFile(template_config_filename);
    ui->section_list->clear();
    ui->section_list->setItemDelegateForColumn(0, new NoEditDelegate(this));
    ui->section_list->setItemDelegateForColumn(1, new NoEditDelegate(this));
    ui->section_list->setItemDelegateForColumn(2, new NoEditDelegate(this));
    ui->section_list->setItemDelegateForColumn(3, new SpinBoxDelegate(this));
    ui->section_list->setItemDelegateForColumn(4, new ComboBoxDelegate(textfield_places, this));

    if (loadFile.open(QIODevice::ReadOnly)) {
        QByteArray saveData = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        QJsonObject obj = loadDoc.object();

        QString report_title = obj["report_title"].toString().trimmed();
        ui->edt_title->setText(report_title);
        ui->edt_image_footer->setText(obj["image_footer"].toString().trimmed());
        ui->edt_image_header->setText(obj["image_header"].toString().trimmed());
        ui->edt_approved->setText(obj["approved_by_field_id"].toString().trimmed());

        ui->text_report_header->setPlainText(obj["static_text_report_header"].toString().trimmed());
        ui->text_page_header->setPlainText(obj["static_text_page_header"].toString().trimmed());
        ui->text_page_footer->setPlainText(obj["static_text_page_footer"].toString().trimmed());
        ui->text_report_footer_above->setPlainText(obj["static_text_report_footer_above_signature"].toString().trimmed());
        ui->text_report_footer_beneath->setPlainText(obj["static_text_report_footer_beneath_signature"].toString().trimmed());

        QJsonArray instance_counts = obj["instance_counts"].toArray();
        if (instance_counts.count() == spinboxes.count()) {
            int i = 0;
            for (auto jitem : instance_counts) {
                if (jitem.isDouble()) {
                    spinboxes[i]->setValue(jitem.toInt());
                }
            }
        }

        QJsonArray section_order = obj["section_order"].toArray();
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
                sl.append("");
                QTreeWidgetItem *item = new QTreeWidgetItem(sl);

                if (obj["print"].toBool()) {
                    item->setCheckState(0, Qt::Checked);
                } else {
                    item->setCheckState(0, Qt::Unchecked);
                }

                if (data_engine.section_uses_variants(sn) || data_engine.section_uses_instances(sn)) {
                    item->setFlags(item_flags_readonly);
                } else {
                    if (obj["as_text_field"].toBool()) {
                        item->setCheckState(2, Qt::Checked);
                    } else {
                        item->setCheckState(2, Qt::Unchecked);
                    }
                    item->setFlags(item_flags_editable);
                    item->setText(3, QString::number(obj["text_field_column_count"].toInt(2)));
                    item->setText(4, obj["text_field_place"].toString());
                }

                ui->section_list->addTopLevelItem(item);
            }
        }
    }

    for (auto sn : sections) {
        QStringList sl;
        sl.append("");
        sl.append(sn);
        sl.append("");
        sl.append("");
        sl.append(textfield_places[0]);
        QTreeWidgetItem *item = new QTreeWidgetItem(sl);
        item->setCheckState(0, Qt::Checked);
        if (data_engine.section_uses_variants(sn) || data_engine.section_uses_instances(sn)) {
            item->setFlags(item_flags_readonly);
        } else {
            item->setCheckState(2, Qt::Unchecked);
            item->setText(3, "2");
            item->setFlags(item_flags_editable);
        }

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
    obj["approved_by_field_id"] = ui->edt_approved->text();

    obj["static_text_report_header"] = ui->text_report_header->toPlainText();
    obj["static_text_page_header"] = ui->text_page_header->toPlainText();
    obj["static_text_page_footer"] = ui->text_page_footer->toPlainText();
    obj["static_text_report_footer_above_signature"] = ui->text_report_footer_above->toPlainText();
    obj["static_text_report_footer_beneath_signature"] = ui->text_report_footer_beneath->toPlainText();

    QJsonArray instance_counts;
    for (auto spinbox : spinboxes) {
        instance_counts.append(spinbox->value());
    }
    obj["instance_counts"] = instance_counts;

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

        bool ok = false;
        auto col_count = item->text(3).toInt(&ok);
        if (!ok) {
            col_count = 2;
        }
        obj["text_field_column_count"] = col_count;

        obj["text_field_place"] = item->text(4);

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

bool DummyDataCreator::get_is_valid_data_engine() {
    return is_valid_data_engine;
}
