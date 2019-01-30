#include "reporthistoryquery.h"
#include "Windows/mainwindow.h"
#include "config.h"
#include "data_engine/data_engine.h"
#include "ui_reporthistoryquery.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDirIterator>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QTreeWidgetItem>
#include <fstream>

ReportHistoryQuery::~ReportHistoryQuery() {
    delete ui;
}

ReportHistoryQuery::ReportHistoryQuery(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReportHistoryQuery) {
    ui->setupUi(this);
    clear_query_pages();
    clear_where_pages();
    add_new_query_page();
    old_stk_report_history_index_m = ui->stk_report_history->currentIndex();
    ui->btn_save_query->setVisible(false);
    ui->btn_save_query_as->setVisible(false);
    load_recent_query_files();
}

void ReportHistoryQuery::load_query_from_file(QString file_name) {
    if (!QFileInfo::exists(file_name)) {
        return;
    }
    query_filename_m = file_name;
    clear_query_pages();
    clear_where_pages();
    report_query_config_file_m.load_from_file(file_name);

    for (auto &report_query : report_query_config_file_m.get_queries_not_const()) {
        QWidget *tool_widget = new QWidget(ui->tb_queries);
        QGridLayout *grid_layout = new QGridLayout(tool_widget);
        report_query_config_file_m.create_new_query_ui(tool_widget, report_query);
        add_new_query_page(report_query, grid_layout, tool_widget);
    }
    for (auto &where : report_query_config_file_m.get_where_fields_not_const()) {
        QWidget *tool_widget = new QWidget(ui->tb_where);
        QGridLayout *grid_layout = new QGridLayout(tool_widget);
        report_query_config_file_m.create_new_where_ui(tool_widget, where);
        add_new_where_page(where, grid_layout, tool_widget);
    }
}

void ReportHistoryQuery::add_new_where_page(const QString &field_name, EntryType field_type) {
    if (field_name == "") {
        return;
    }
    QWidget *tool_widget = new QWidget(ui->tb_where);
    QGridLayout *grid_layout = new QGridLayout(tool_widget);
    auto &report_where = report_query_config_file_m.add_new_where(tool_widget, field_name, field_type);
    ui->tb_where->addItem(tool_widget, field_name + "(" + field_type.to_string() + ")");
    tool_widget->setProperty("field_name", field_name);
    ui->tb_where->setCurrentIndex(ui->tb_where->count() - 1);

    (void)report_where;
    (void)grid_layout;
}

void ReportHistoryQuery::add_new_where_page(ReportQueryWhereField &report_where, QGridLayout *grid_layout, QWidget *tool_widget) {
    ui->tb_where->addItem(tool_widget, report_where.field_name_m + "(" + report_where.field_type_m.to_string() + ")");
    tool_widget->setProperty("field_name", report_where.field_name_m);
    ui->tb_where->setCurrentIndex(ui->tb_where->count() - 1);

    (void)grid_layout;
}

void ReportHistoryQuery::add_new_query_page() {
    QWidget *tool_widget = new QWidget(ui->tb_queries);
    QGridLayout *grid_layout = new QGridLayout(tool_widget);
    ReportQuery &report_query = report_query_config_file_m.add_new_query(tool_widget);
    add_new_query_page(report_query, grid_layout, tool_widget);
}

void ReportHistoryQuery::add_new_query_page(ReportQuery &report_query, QGridLayout *grid_layout, QWidget *tool_widget) {
    ui->tb_queries->addItem(tool_widget, report_query.get_data_engine_source_file());

    reduce_tb_query_path();

    connect(report_query.btn_query_report_file_browse_m, &QToolButton::clicked, [this, report_query](bool checked) {
        (void)checked; //
        const auto selected_dir = QFileDialog::getExistingDirectory(this, QObject::tr("Open report directory"), report_query.edt_query_report_folder_m->text(),
                                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!selected_dir.isEmpty()) {
            report_query.edt_query_report_folder_m->setText(selected_dir);
        }
    });

    connect(report_query.btn_query_data_engine_source_file_browse_m, &QToolButton::clicked, [this, report_query](bool checked) {
        (void)checked; //
        QFileDialog dialog(this);
        dialog.setDirectory(report_query.edt_query_data_engine_source_file_m->text());
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setNameFilter(tr("Data Engine Input files (*.json)"));
        if (dialog.exec()) {
            report_query.edt_query_data_engine_source_file_m->setText(dialog.selectedFiles()[0]);
        }
    });

    connect(report_query.btn_query_add_m, &QToolButton::clicked, [this](bool checked) {
        (void)checked; //
        add_new_query_page();
    });

    connect(report_query.btn_query_del_m, &QToolButton::clicked, [this, tool_widget](bool checked) {
        (void)checked; //
        remove_query_page(tool_widget);
    });

    connect(report_query.edt_query_data_engine_source_file_m, &QLineEdit::textChanged, [this, tool_widget](const QString &arg) {
        int index = ui->tb_queries->indexOf(tool_widget);
        ui->tb_queries->setItemText(index, arg);
        reduce_tb_query_path();
    });

    (void)grid_layout;
}

void ReportHistoryQuery::on_tree_query_fields_itemDoubleClicked(QTreeWidgetItem *item, int column) {
    (void)column;
    QString field_name = item->data(0, Qt::UserRole).toString();
    if (field_name != "") {
        QString field_type_str = item->text(1);
        EntryType field_type(field_type_str);
        add_new_where_page(field_name, field_type);
        item->setCheckState(0, Qt::Checked);
    }
}

void ReportHistoryQuery::on_toolButton_3_clicked() {
    on_tree_query_fields_itemDoubleClicked(ui->tree_query_fields->currentItem(), 0);
}

void ReportHistoryQuery::remove_query_page(QWidget *tool_widget) {
    int current_index = ui->tb_where->indexOf(tool_widget);
    report_query_config_file_m.remove_query(current_index);
    ui->tb_queries->removeItem(current_index);
    delete tool_widget;
}

void ReportHistoryQuery::clear_query_pages() {
    while (ui->tb_queries->count()) {
        QWidget *widget = ui->tb_queries->widget(0);
        delete widget;
        ui->tb_queries->removeItem(0);
    }
}

void ReportHistoryQuery::remove_where_page(int index) {
    QWidget *widget = ui->tb_where->widget(index);
    delete widget;
    ui->tb_queries->removeItem(index);
}

void ReportHistoryQuery::clear_where_pages() {
    while (ui->tb_where->count()) {
        remove_where_page(0);
    }
}

void ReportHistoryQuery::on_btn_next_clicked() {
    int i = ui->stk_report_history->currentIndex() + 1;
    if (i < ui->stk_report_history->count()) {
        ui->stk_report_history->setCurrentIndex(i);
    }
}

void ReportHistoryQuery::on_btn_back_clicked() {
    int i = ui->stk_report_history->currentIndex() - 1;
    if (i > -1) {
        ui->stk_report_history->setCurrentIndex(i);
    }
}

bool ReportHistoryQuery::load_select_ui_to_query() {
    for (int i = 0; i < ui->tree_query_fields->topLevelItemCount(); i++) {
        QTreeWidgetItem *top_level_item = ui->tree_query_fields->topLevelItem(i);
        auto &query = report_query_config_file_m.find_query_by_table_name(top_level_item->text(0));
        query.select_field_names_m.clear();

        for (int j = 0; j < top_level_item->childCount(); j++) {
            if (top_level_item->child(j)->text(0) == "report") {
                QTreeWidgetItem *report_item = top_level_item->child(j);

                for (int k = 0; k < report_item->childCount(); k++) {
                    QTreeWidgetItem *section_item = report_item->child(k);
                    QString section_name = section_item->text(0);
                    for (int m = 0; m < section_item->childCount(); m++) {
                        QTreeWidgetItem *field_item = section_item->child(m);
                        if (field_item->checkState(0) == Qt::Checked) {
                            QString field_name = field_item->text(0);
                            query.select_field_names_m.append("report/" + section_name + "/" + field_name);
                            qDebug() << "report/" + section_name + "/" + field_name;
                        }
                    }
                }
            } else if (top_level_item->child(j)->text(0) == "general") {
                QTreeWidgetItem *general_item = top_level_item->child(j);
                for (int k = 0; k < general_item->childCount(); k++) {
                    QTreeWidgetItem *field_item = general_item->child(k);
                    if (field_item->checkState(0) == Qt::Checked) {
                        QString field_name = field_item->text(0);
                        query.select_field_names_m.append("general/" + field_name);
                        qDebug() << "general/" + field_name;
                    }
                }
            } else {
                assert(0);
            }
        }
    }
    QList<ReportQueryWhereField> &where_fields = report_query_config_file_m.get_where_fields_not_const();
    for (auto &where_field : where_fields) {
        try {
            where_field.load_values_from_plain_text();
            where_field.lbl_warning_m->setText("");
            QIcon icon_warn = QIcon{};
            int index = ui->tb_where->indexOf(where_field.parent_m);
            ui->tb_where->setItemIcon(index, icon_warn);
        } catch (WhereFieldInterpretationError &e) {
            //   qDebug() << e.what();
            QIcon icon_warn = QIcon{"://src/icons/warning_16.ico"};
            where_field.lbl_warning_m->setText("Error: " + QString().fromStdString(e.what()));
            QPalette palette = where_field.lbl_warning_m->palette();
            palette.setColor(where_field.lbl_warning_m->backgroundRole(), Qt::darkRed);
            palette.setColor(where_field.lbl_warning_m->foregroundRole(), Qt::darkRed);
            where_field.lbl_warning_m->setPalette(palette);
            int index = ui->tb_where->indexOf(where_field.parent_m);
            ui->tb_where->setItemIcon(index, icon_warn);
            ui->tb_where->setCurrentIndex(index);
            QMessageBox::warning(MainWindow::mw, QString("Value parsing error"), QString("There is an error in the where-values: %1").arg(e.what()));
            return false;
        }
    }
    return true;
}

void ReportHistoryQuery::on_stk_report_history_currentChanged(int arg1) {
    if (arg1 == 0) {
        load_recent_query_files();
    }
    if ((old_stk_report_history_index_m == 0) && (arg1 == 1)) {
        ui->tree_query_fields->clear();
        //T:/qt/crystalTestFramework/tests/scripts/report_query/data_engine_source_1.json
        const Qt::ItemFlags item_flags_checkable = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;

        report_query_config_file_m.set_table_names();

        for (auto &query : report_query_config_file_m.get_queries_not_const()) {
            QTreeWidgetItem *root_item = new QTreeWidgetItem(QStringList{query.get_table_name()});
            ui->tree_query_fields->addTopLevelItem(root_item);

            const DataEngineSourceFields &fields = query.get_data_engine_fields();
            QTreeWidgetItem *root_general_widget = new QTreeWidgetItem(root_item, QStringList{"general"});
            QTreeWidgetItem *root_report_widget = new QTreeWidgetItem(root_item, QStringList{"report"});

            for (const auto &general_field : fields.general_fields_m) {
                QString typ_name = "";
                QTreeWidgetItem *general_entry =
                    new QTreeWidgetItem(root_general_widget, QStringList{general_field.field_name_m, general_field.field_type_m.to_string()});
                QString fn = "general/" + general_field.field_name_m;
                if (query.select_field_names_m.contains(fn)) {
                    general_entry->setCheckState(0, Qt::Checked);
                } else {
                    general_entry->setCheckState(0, Qt::Unchecked);
                }
                general_entry->setFlags(item_flags_checkable);
                general_entry->setData(0, Qt::UserRole, fn);
                //  qDebug() << "general/" + general_field.field_name;
            }

            for (const auto &section_name : fields.report_fields_m.keys()) {
                QTreeWidgetItem *report_section_entry = new QTreeWidgetItem(root_report_widget, QStringList{section_name});
                for (const auto &data_engine_field : fields.report_fields_m.value(section_name)) {
                    QTreeWidgetItem *report_entry =
                        new QTreeWidgetItem(report_section_entry, QStringList{data_engine_field.field_name_m, data_engine_field.field_type_m.to_string()});

                    QString fn = "report/" + section_name + "/" + data_engine_field.field_name_m;
                    if (query.select_field_names_m.contains(fn)) {
                        report_entry->setCheckState(0, Qt::Checked);
                    } else {
                        report_entry->setCheckState(0, Qt::Unchecked);
                    }
                    report_entry->setFlags(item_flags_checkable);
                    report_entry->setData(0, Qt::UserRole, fn);
                    //    qDebug() << "report/" + section_name + "/" + data_engine_field.field_name;
                }
            }
        }
        ui->tree_query_fields->expandAll();
    } else if ((old_stk_report_history_index_m == 1) && (arg1 == 2)) {
        if (load_select_ui_to_query()) {
            ReportDatabase query_result = report_query_config_file_m.execute_query(this);
            query_result.join();
            ReportTable *joined_table = query_result.get_root_table();

            QStringList row_titles;
            auto col_keys = joined_table->get_field_names();
            for (int key : col_keys.uniqueKeys()) {
                row_titles.append(query_result.get_field_name_by_int_key(key));
            }
            row_titles = reduce_path(row_titles);

            ui->tableWidget->clear();
            ui->tableWidget->setColumnCount(row_titles.count());
            QStringList sl = reduce_path(row_titles);
            ui->tableWidget->setHorizontalHeaderLabels(sl);
            ui->tableWidget->setRowCount(1);

            const QList<ReportTableRow> &joined_rows = joined_table->get_rows();

            int row_index = 0;
            for (auto &row : joined_rows) {
                int col_index = 0;
                for (int &col_key : col_keys.uniqueKeys()) {
                    const QVariant &col_val = row.row_m.value(col_key);

                    QString s;
                    if (col_val.canConvert<DataEngineDateTime>()) {
                        s = col_val.value<DataEngineDateTime>().str();
                    } else {
                        s = col_val.toString();
                    }
                    auto *item = new MyTableWidgetItem(s, 0);
                    item->setData(Qt::UserRole, col_val);
                    ui->tableWidget->setItem(row_index, col_index, item);
                    col_index++;
                }
                row_index++;
                ui->tableWidget->setRowCount(row_index + 1);
            }
        } else {
            ui->stk_report_history->setCurrentIndex(old_stk_report_history_index_m);
            arg1 = old_stk_report_history_index_m;
        }
    }

    old_stk_report_history_index_m = arg1;
    ui->btn_save_query->setVisible((arg1 == 1) || (arg1 == 2));
    ui->btn_save_query->setToolTip(QObject::tr("Saves to file: ") +
                                   QDir(QSettings{}.value(Globals::report_history_query_path, "").toString()).relativeFilePath(query_filename_m));
    ui->btn_save_query_as->setVisible(ui->btn_save_query->isVisible());
    ui->btn_save_query_as->setEnabled(QFileInfo::exists(query_filename_m));
    ui->btn_next->setEnabled(arg1 < 2);
    ui->btn_back->setEnabled(arg1 > 0);
}

void ReportHistoryQuery::on_btn_result_export_clicked() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Report query result(*.csv)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDirectory(QSettings{}.value(Globals::report_history_query_path, "").toString());
    if (dialog.exec()) {
        const QChar SEPEARTOR = '\t';
        QString file_name = dialog.selectedFiles()[0];
        QFile csv_file(file_name);
        if (csv_file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream csv_stream(&csv_file);

            for (int r = 0; r < ui->tableWidget->rowCount(); r++) {
                if (r == 0) {
                    for (int c = 0; c < ui->tableWidget->columnCount(); c++) {
                        csv_stream << ui->tableWidget->horizontalHeaderItem(c)->text();
                        if (c < ui->tableWidget->columnCount() - 1) {
                            csv_stream << SEPEARTOR;
                        }
                    }
                    csv_stream << "\n";
                }
                for (int c = 0; c < ui->tableWidget->columnCount(); c++) {
                    QVariant data = ui->tableWidget->item(r, c)->data(Qt::UserRole);
                    QString val;
                    if (data.type() == QVariant::String) {
                        val = "\"" + data.toString() + "\"";
                    } else {
                        val = ui->tableWidget->item(r, c)->text();
                    }
                    csv_stream << val;
                    if (c < ui->tableWidget->columnCount() - 1) {
                        csv_stream << SEPEARTOR;
                    }
                }
                csv_stream << "\n";
            }
            csv_file.close();
        }
    }
}

void ReportHistoryQuery::on_btn_close_clicked() {
    close();
}

void ReportHistoryQuery::on_tree_query_fields_itemClicked(QTreeWidgetItem *item, int column) {
    if (column == 0) {
        QString field_name = item->data(0, Qt::UserRole).toString();
        if ((item->checkState(0) == Qt::Unchecked) && (field_name.count())) {
            if (report_query_config_file_m.remove_where(field_name)) {
                for (int i = 0; i < ui->tb_where->count(); i++) {
                    if (ui->tb_where->widget(i)->property("field_name").toString() == field_name) {
                        remove_where_page(i);
                        break;
                    }
                }
            }
        }
    }
}

void ReportHistoryQuery::on_btn_where_del_clicked() {
    QWidget *tool_widget = ui->tb_where->currentWidget();
    if (tool_widget) {
        QString field_name = tool_widget->property("field_name").toString();
        report_query_config_file_m.remove_where(field_name);
        remove_where_page(ui->tb_where->currentIndex());
    }
}

void ReportQueryConfigFile::create_new_where_ui(QWidget *parent, ReportQueryWhereField &report_where) {
    if (parent) {
        QGridLayout *gl = dynamic_cast<QGridLayout *>(parent->layout());
        report_where.plainTextEdit_m = new QPlainTextEdit();
        report_where.parent_m = parent;
        QStringList text;
        for (auto &where_seg : report_where.field_values_m) {
            for (auto &where_segment_value : where_seg.values_m) {
                switch (report_where.field_type_m.t) {
                    case EntryType::Bool: {
                        bool val = where_segment_value.toBool();
                        if (val) {
                            text.append("true");
                        } else {
                            text.append("false");
                        }
                    } break;
                    case EntryType::Number: {
                        double val = where_segment_value.toDouble();
                        text.append(QString::number(val));
                    } break;
                    case EntryType::Text:
                        text.append(where_segment_value.toString());
                        break;
                    case EntryType::DateTime: {
                        DataEngineDateTime dt = where_segment_value.value<DataEngineDateTime>();
                        text.append(dt.str());
                    } break;
                    case EntryType::Unspecified: {
                        assert(0);
                    } break;
                    case EntryType::Reference: {
                        assert(0);
                    } break;
                }
            }
            if (where_seg.include_greater_values_till_next_entry_m) {
                text.append("*");
            }
        }
        for (auto &t : text) {
            report_where.plainTextEdit_m->appendPlainText(t);
        }
        gl->addWidget(report_where.plainTextEdit_m, 0, 0);
        report_where.lbl_warning_m = new QLabel();
        gl->addWidget(report_where.lbl_warning_m, 1, 0);
    }
};

ReportQueryWhereField &ReportQueryConfigFile::add_new_where(QWidget *parent, QString field_name, EntryType field_type) {
    ReportQueryWhereField report_where{};
    report_where.field_name_m = field_name;
    report_where.field_type_m = field_type;
    create_new_where_ui(parent, report_where);
    query_where_fields_m.append(report_where);
    return query_where_fields_m.last();
}

bool ReportQueryConfigFile::remove_where(QString field_name) {
    for (int i = 0; i < query_where_fields_m.count(); i++) {
        if (query_where_fields_m[i].field_name_m == field_name) {
            query_where_fields_m.removeAt(i);
            return true;
        }
    }
    return false;
}

void ReportQueryConfigFile::create_new_query_ui(QWidget *parent, ReportQuery &report_query) {
    if (parent) {
        QGridLayout *gl = dynamic_cast<QGridLayout *>(parent->layout());

        QLabel *lbl_data_engine_source = new QLabel();
        lbl_data_engine_source->setText(QObject::tr("Data engine source file:"));
        gl->addWidget(lbl_data_engine_source, 0, 0);

        report_query.edt_query_data_engine_source_file_m = new QLineEdit();
        report_query.edt_query_data_engine_source_file_m->setText(report_query.get_data_engine_source_file());
        gl->addWidget(report_query.edt_query_data_engine_source_file_m, 0, 1);

        report_query.btn_query_data_engine_source_file_browse_m = new QToolButton();
        report_query.btn_query_data_engine_source_file_browse_m->setText("..");
        gl->addWidget(report_query.btn_query_data_engine_source_file_browse_m, 0, 2);

        QLabel *lbl_report_path = new QLabel();
        lbl_report_path->setText(QObject::tr("Test report seach folder:"));
        gl->addWidget(lbl_report_path, 1, 0);

        report_query.edt_query_report_folder_m = new QLineEdit();
        report_query.edt_query_report_folder_m->setText(report_query.report_path_m);

        gl->addWidget(report_query.edt_query_report_folder_m, 1, 1);

        report_query.btn_query_report_file_browse_m = new QToolButton();
        report_query.btn_query_report_file_browse_m->setText("..");
        gl->addWidget(report_query.btn_query_report_file_browse_m, 1, 2);

        QHBoxLayout *layout_add_del = new QHBoxLayout();
        layout_add_del->addStretch(1);
        report_query.btn_query_add_m = new QToolButton();
        report_query.btn_query_add_m->setText("+");
        layout_add_del->addWidget(report_query.btn_query_add_m);

        report_query.btn_query_del_m = new QToolButton();
        report_query.btn_query_del_m->setText("-");
        layout_add_del->addWidget(report_query.btn_query_del_m);
        gl->addLayout(layout_add_del, 2, 1, 2, 1);
        gl->setRowStretch(4, 1);
    }
}

ReportQuery &ReportQueryConfigFile::add_new_query(QWidget *parent) {
    ReportQuery report_query{};
    create_new_query_ui(parent, report_query);
    report_queries_m.append(report_query);
    return report_queries_m.last();
}

#if 0
ReportQuery &ReportQueryConfigFile::find_query_by_source_file(QString data_engine_source_file) {
    for (auto &query : report_queries_m) {
        if (query.get_data_engine_source_file().endsWith(data_engine_source_file)) {
            return query;
        }
    }
    throw std::runtime_error("ReportQueryConfigFile: can not find query with " + data_engine_source_file.toStdString() + " as filename.");
}
#endif

ReportQuery &ReportQueryConfigFile::find_query_by_table_name(QString table_name) {
    for (auto &query : report_queries_m) {
        if (query.get_table_name() == table_name) {
            return query;
        }
    }
    throw std::runtime_error("ReportQueryConfigFile: can not find query with " + table_name.toStdString() + " as tablename.");
}

ReportQuery &ReportQueryConfigFile::find_query_by_id(int id) {
    for (auto &query : report_queries_m) {
        if (query.id_m == id) {
            return query;
        }
    }
    throw std::runtime_error("ReportQueryConfigFile: can not find query with " + QString::number(id).toStdString() + " as tableindex.");
}

void ReportQueryConfigFile::build_query_link_references() {
    for (auto &query : report_queries_m) {
        if (query.link.other_id >= 0) {
            auto &query_to_probe = find_query_by_id(query.link.other_id);
            query.link.other_report_query = &query_to_probe;
        }
        //for (auto &query_to_probe : report_queries_m) {
        //  if (query.link.other_id == query_to_probe.id_m) {
        //    query.link.other_report_query = &query_to_probe;
        //  break;
        //}
        // }
    }
    reference_links_built_m = true;
}

void ReportQueryConfigFile::remove_query(int index) {
    report_queries_m.removeAt(index);
}

ReportDatabase ReportQueryConfigFile::execute_query(QWidget *parent) const {
    QProgressDialog progress(
        QObject::tr("Scanning files..\nIf this gets too slow, ask your systemadmin to set up a MongoDB server and the developer to use it."),
        QObject::tr("Scaning files.."), 0, 100, parent);
    progress.setCancelButton(nullptr);
    progress.setWindowModality(Qt::WindowModal);

    QList<ReportLink> rl = scan_folder_for_reports("");
    progress.setMaximum(rl.count());
    return filter_and_select_reports(rl, &progress);
}

ReportQueryConfigFile::ReportQueryConfigFile() {}

ReportQueryConfigFile::~ReportQueryConfigFile() {}

void ReportQueryConfigFile::load_from_file(QString file_name) {
    if ((file_name == "") || !QFile::exists(file_name)) {
        QString msg = QString{"report query config file %1 does not exist."}.arg(file_name);
        throw sol::error(msg.toStdString());
    }
    QFile loadFile(file_name);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        QString msg = QString{"cant open report query config file %1"}.arg(file_name);
        throw sol::error(msg.toStdString());
    }
    QByteArray config_file_content = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(config_file_content));

    QJsonObject doc_obj = loadDoc.object();
    QJsonArray js_where_fields = doc_obj["where_fields"].toArray();

    query_where_fields_m.clear();
    for (auto js_where_field : js_where_fields) {
        ReportQueryWhereField where_field;
        QJsonObject js_where_field_obj = js_where_field.toObject();
        where_field.field_name_m = js_where_field_obj["field_name"].toString();
        QString s = js_where_field_obj["field_type"].toString();
        where_field.field_type_m = EntryType(s);
        where_field.incremention_selector_expression_ = js_where_field_obj["incremention_selector_expression"].toString();
        QJsonArray js_conditions = js_where_field_obj["conditions"].toArray();

        for (auto js_condition : js_conditions) {
            ReportQueryWhereFieldValues field_values;
            QJsonObject obj = js_condition.toObject();
            field_values.include_greater_values_till_next_entry_m = obj["include_greater_values_till_next_entry"].toBool();
            QJsonArray js_values = obj["values"].toArray();
            for (auto value : js_values) {
                field_values.values_m.append(value.toInt());
            }
            where_field.field_values_m.append(field_values);
        }
        query_where_fields_m.append(where_field);
    }

    QJsonArray js_queries = doc_obj["queries"].toArray();
    report_queries_m.clear();
    for (const auto js_query : js_queries) {
        ReportQuery rq;
        const auto js_query_obj = js_query.toObject();
        rq.report_path_m = js_query_obj["report_path"].toString();
        rq.set_data_engine_source_file(js_query_obj["data_engine_source_file"].toString());
        rq.id_m = js_query_obj["id"].toInt(-1);
        QJsonObject link_obj = js_query_obj["link"].toObject();
        rq.link.other_id = link_obj["other_query_id"].toInt(-1);
        rq.link.other_field_name = link_obj["other_field"].toString();
        rq.link.me_field_name = link_obj["me"].toString();
        QJsonArray js_select_field_name = js_query_obj["select_field_names"].toArray();
        for (const auto v : js_select_field_name) {
            rq.select_field_names_m.append(v.toString());
        }
        report_queries_m.append(rq);
    }
    set_table_names();
    build_query_link_references();
}

void ReportQueryConfigFile::set_table_names() {
    //set_table_names by reducing the data_source_paths
    QStringList sl;
    for (auto &query : get_queries_not_const()) {
        query.update_from_gui();
        sl.append(query.get_table_name_suggestion());
    }
    sl = reduce_path(sl);
    int i = 0;
    for (auto &query : get_queries_not_const()) {
        query.set_table_name(sl[i]);
        i++;
    }
}

void ReportQueryConfigFile::save_to_file(QString file_name) {
    QJsonArray js_where_array;
    QJsonArray js_query_array;
    for (auto &where : query_where_fields_m) {
        QJsonArray js_where_value_segment_array;
        for (auto &value_segment : where.field_values_m) {
            QJsonArray js_where_value_array;
            for (auto &value : value_segment.values_m) {
                switch (where.field_type_m.t) {
                    case EntryType::Number:
                        js_where_value_array.append(value.toDouble());
                        break;
                    case EntryType::DateTime: {
                        DataEngineDateTime dt = value.value<DataEngineDateTime>();
                        js_where_value_array.append(dt.str());
                    } break;
                    case EntryType::Text:
                        js_where_value_array.append(value.toString());
                        break;
                    case EntryType::Bool:
                        js_where_value_array.append(value.toBool());
                        break;
                    case EntryType::Unspecified:
                        assert(0);
                        break;
                    case EntryType::Reference:
                        assert(0);
                        break;
                }
            }
            QJsonObject js_where_value_segment_obj;
            js_where_value_segment_obj["include_greater_values_till_next_entry"] = value_segment.include_greater_values_till_next_entry_m;
            js_where_value_segment_obj["values"] = js_where_value_array;
            js_where_value_segment_array.append(js_where_value_segment_obj);
        }
        QJsonObject js_where_entry;
        js_where_entry["conditions"] = js_where_value_segment_array;
        js_where_entry["field_name"] = where.field_name_m;
        js_where_entry["field_type"] = where.field_type_m.to_string();
        js_where_array.append(js_where_entry);
    }
    for (auto &query : report_queries_m) {
        QJsonObject js_query_entry;
        js_query_entry["report_path"] = query.report_path_m;
        js_query_entry["data_engine_source_file"] = query.get_data_engine_source_file();
        QJsonArray js_query_select_array;
        for (auto &select_file_name : query.select_field_names_m) {
            js_query_select_array.append(select_file_name);
        }
        js_query_entry["select_field_names"] = js_query_select_array;
        js_query_array.append(js_query_entry);
    }
    QJsonObject obj;
    obj["queries"] = js_query_array;
    obj["where_fields"] = js_where_array;
    QFile saveFile(file_name);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return;
    }
    QJsonDocument saveDoc(obj);
    saveFile.write(saveDoc.toJson());
}

const QList<ReportQuery> &ReportQueryConfigFile::get_queries() const {
    return report_queries_m;
}

QList<ReportQuery> &ReportQueryConfigFile::get_queries_not_const() {
    return report_queries_m;
}

const QList<ReportQueryWhereField> &ReportQueryConfigFile::get_where_fields() {
    return query_where_fields_m;
}

QList<ReportQueryWhereField> &ReportQueryConfigFile::get_where_fields_not_const() {
    return query_where_fields_m;
}

ReportDatabase ReportQueryConfigFile::filter_and_select_reports(const QList<ReportLink> &report_file_list, QProgressDialog *progress_dialog) const {
    ReportDatabase report_database;
    int progress = 0;

    assert(reference_links_built_m);
    for (const auto &query : get_queries()) {
        QString other_table;
        if (query.link.other_report_query) {
            other_table = query.link.other_report_query->get_table_name();
        }

        QString other_field_name = other_table + "/" + query.link.other_field_name;
        if (other_field_name == "/") {
            other_field_name = "";
        }

        QString me_field_name = query.link.me_field_name;
        if (me_field_name != "") {
            me_field_name = query.get_table_name() + "/" + me_field_name;
        }

        ReportTable *table = report_database.new_table(query.get_table_name(), me_field_name, other_field_name);

        QMap<int, QString> field_name_keys;
        for (const QString &field_name : query.select_field_names_m) {
            QString fn = query.get_table_name() + "/" + field_name;
            field_name_keys.insert(report_database.get_int_key_by_field_name_not_const(fn), fn);
        }
        table->set_field_name_keys(field_name_keys);
    }
    report_database.build_link_tree();

    for (const auto &report_link : report_file_list) {
        progress++;
        if (progress_dialog) {
            progress_dialog->setValue(progress);
        }
        ReportFile report_file;
        report_file.load_from_file(report_link.report_path_m);
        if (only_successful_reports_m) {
            if ((report_file.get_field_value("general/everything_complete").toBool() == false) ||
                (report_file.get_field_value("general/everything_in_range").toBool()) == false) {
                continue;
            }
        }

        bool match = true;
        QString index_key;
        for (const auto &report_query_where_field : query_where_fields_m) {
            auto value = report_file.get_field_value(report_query_where_field.field_name_m);
            index_key = index_key + value.toString();
            if (report_query_where_field.matches_value(value) == false) {
                match = false;
                break;
            }
        }

        if (match) {
            QVariant time_stamp_var = report_file.get_field_value("general/datetime_str");
            DataEngineDateTime time_stamp;
            if (time_stamp_var.canConvert<DataEngineDateTime>()) {
                time_stamp = time_stamp_var.value<DataEngineDateTime>();
            }

            QMap<QString, QVariant> pre_row;
            for (auto select_field_name : report_link.query_m.select_field_names_m) {
                QString field_name = report_link.query_m.get_table_name() + "/" + select_field_name;
                pre_row.insert(field_name, report_file.get_field_value(select_field_name));
            }
            ReportTable *report_table = report_database.get_table(report_link.query_m.get_table_name());
            QMap<int, QVariant> table_row = report_database.translate_row_to_int_key(pre_row);
            report_table->append_row(table_row, time_stamp.dt());
        }
    }
    return report_database;
}

QList<ReportLink> ReportQueryConfigFile::scan_folder_for_reports(const QString &base_dir_str) const {
    QList<ReportLink> result;
    for (const auto &query : report_queries_m) {
        QDir base_dir{base_dir_str};
        QString report_search_path = query.report_path_m;
        QDir report_path{report_search_path};
        if (report_path.isRelative()) {
            report_search_path = base_dir.absoluteFilePath(report_search_path);
        }
        QDirIterator dit{report_search_path, QStringList{} << "*.json", QDir::Files, QDirIterator::Subdirectories};
        while (dit.hasNext()) {
            const auto &file_path = dit.next();
            result.append(ReportLink{file_path, //
                                     query});
        }
    }
    return result;
}

ReportFile::ReportFile() {}

ReportFile::~ReportFile() {}

void ReportFile::load_from_file(QString file_name) {
    if ((file_name == "") || !QFile::exists(file_name)) {
        QString msg = QString{"report query config file %1 does not exist."}.arg(file_name);
        throw sol::error(msg.toStdString());
    }
    QFile loadFile(file_name);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        QString msg = QString{"cant open report query config file %1"}.arg(file_name);
        throw sol::error(msg.toStdString());
    }
    QByteArray report_file_content = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(report_file_content));
    js_report_object_m = loadDoc.object();
}

QVariant ReportFile::get_field_value(QString field_name) {
    auto field_path = field_name.split("/");
    const bool is_report_field = field_path[0] == "report";
    auto last_field_name_component = field_path.last();
    field_path.removeLast();
    if (is_report_field) {
        field_path[0] = "sections";
    }
    QJsonObject js_value_obj = js_report_object_m;
    for (const auto &field_path_component : field_path) {
        js_value_obj = js_value_obj[field_path_component].toObject();
    }
    if (is_report_field) {
        QJsonArray js_instance = js_value_obj["instances"].toArray();
        QJsonArray js_fields_in_section = js_instance[0].toObject()["fields"].toArray();
        for (auto field : js_fields_in_section) {
            auto js_field_obj = field.toObject();
            auto field_name_to_probe = js_field_obj["name"].toString();
            if (field_name_to_probe == last_field_name_component) {
                auto js_numeric_obj = js_field_obj["numeric"].toObject();
                auto js_text_obj = js_field_obj["text"].toObject();
                auto js_bool_obj = js_field_obj["bool"].toObject();
                auto js_datetime_obj = js_field_obj["datetime"].toObject();

                if (!js_numeric_obj.isEmpty()) {
                    return QVariant(js_numeric_obj["actual"].toObject()["value"].toDouble());
                } else if (!js_text_obj.isEmpty()) {
                    return QVariant(js_text_obj["actual"].toObject()["value"].toString());
                } else if (!js_bool_obj.isEmpty()) {
                    return QVariant(js_bool_obj["actual"].toObject()["value"].toBool());
                } else if (!js_datetime_obj.isEmpty()) {
                    auto ms_since_epoch = js_datetime_obj["actual"].toObject()["ms_since_epoch"].toDouble();
                    DataEngineDateTime dt(QDateTime::fromMSecsSinceEpoch(round(ms_since_epoch)));
                    QVariant res;
                    res.setValue<DataEngineDateTime>(dt);
                    return res;
                }
            }
        }
    } else {
        QVariant result = js_value_obj[last_field_name_component].toVariant();
        if (result.type() == QVariant::String) {
            DataEngineDateTime dt(result.toString());

            if (dt.isValid()) {
                result.setValue<DataEngineDateTime>(dt);
            }
        }
        return result;
    }
    return QVariant();
}

bool ReportQueryWhereField::matches_value(QVariant value) const {
    for (int i = 0; i < field_values_m.count(); i++) {
        auto field_value = field_values_m[i];
        if (field_value.values_m.contains(value)) {
            return true;
        }

        if (field_value.include_greater_values_till_next_entry_m) {
            const bool is_num = (field_type_m.t == EntryType::Number);
            const bool is_date = (field_type_m.t == EntryType::DateTime);
            if (is_num || is_date) {
                if (i == field_values_m.count() - 1) { //is last condition?
                    if (value > field_value.values_m.last()) {
                        return true;
                    }
                } else {
                    if ((value > field_value.values_m.last()) && (value < field_values_m[i + 1].values_m.first())) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void ReportQueryWhereField::load_values_from_plain_text() {
    QString plainTextEditContents = plainTextEdit_m->toPlainText();
    QStringList plain_text = plainTextEditContents.split("\n");
    field_values_m.clear();

    switch (field_type_m.t) {
        case EntryType::Number: {
            ReportQueryWhereFieldValues value;
            for (auto s : plain_text) {
                s = s.trimmed();
                if (s == "") {
                    continue;
                }
                if (s == "*") {
                    value.include_greater_values_till_next_entry_m = true;
                    field_values_m.append(value);
                    value = ReportQueryWhereFieldValues();
                    continue;
                }
                bool parse_ok = false;
                double num_value = s.toDouble(&parse_ok);
                if (!parse_ok) {
                    throw WhereFieldInterpretationError("cannot convert '" + s + "' to number");
                }
                value.values_m.append(QVariant(num_value));
            }
            if (value.values_m.count()) {
                field_values_m.append(value);
            }
        } //
        break;
        case EntryType::DateTime: //
        {
            ReportQueryWhereFieldValues value;
            for (auto s : plain_text) {
                s = s.trimmed();
                if (s == "") {
                    continue;
                }
                if (s == "*") {
                    value.include_greater_values_till_next_entry_m = true;
                    field_values_m.append(value);
                    value = ReportQueryWhereFieldValues();
                    continue;
                }
                DataEngineDateTime datetime_value(s);
                if (!datetime_value.isValid()) {
                    throw WhereFieldInterpretationError("cannot convert '" + s + "' to date time. Allowed formats: " +
                                                        DataEngineDateTime::allowed_formats().join(", "));
                }
                QVariant val;
                val.setValue<DataEngineDateTime>(datetime_value);
                value.values_m.append(val);
            }
            if (value.values_m.count()) {
                field_values_m.append(value);
            }
            if (field_values_m.count() == 0) {
                throw WhereFieldInterpretationError("Match values for " + field_name_m + " are not yet defined.");
            }
        } //
        break;
        case EntryType::Text: {
            ReportQueryWhereFieldValues value;
            for (auto s : plain_text) {
                s = s.trimmed();
                if (s == "") {
                    continue;
                }
                if (s == "*") {
                    throw WhereFieldInterpretationError("Wild cards for text fields are not allowed.");
                }

                value.values_m.append(s);
            }
            if (value.values_m.count()) {
                field_values_m.append(value);
            }
            if (field_values_m.count() == 0) {
                throw WhereFieldInterpretationError("Match values for " + field_name_m + " are not yet defined.");
            }
        } //
        break;
        case EntryType::Bool: //
        {
            ReportQueryWhereFieldValues value;
            int counter = 0;
            for (auto s : plain_text) {
                s = s.trimmed();
                if (s == "") {
                    continue;
                }
                QStringList true_words{"true", "ja", "yes", "wahr"};
                QStringList false_words{"false", "nein", "no", "falsch"};
                if (true_words.contains(s.toLower())) {
                    value.values_m.append(QVariant(true));
                } else if (false_words.contains(s.toLower())) {
                    value.values_m.append(QVariant(false));
                } else {
                    throw WhereFieldInterpretationError("cannot convert '" + s + "' to bool. Allowed formats: " + true_words.join(", ") + " / " +
                                                        false_words.join(", "));
                }

                value.values_m.append(s);
                counter++;
            }
            if (value.values_m.count()) {
                field_values_m.append(value);
            }
            if (field_values_m.count() == 0) {
                throw WhereFieldInterpretationError("Match values for " + field_name_m + " are not yet defined.");
            }
            if (counter > 1) {
                throw WhereFieldInterpretationError("Bool fields only allow one match value");
            }
        } //
        break;
        case EntryType::Unspecified: //
            assert(0);
            break;
        case EntryType::Reference: //
            assert(0);
            break;
    }
}

QString ReportQuery::get_data_engine_source_file() const {
    return data_engine_source_file_m;
}

void ReportQuery::set_data_engine_source_file(const QString &data_engine_source_file) {
    data_engine_source_file_m = data_engine_source_file;
}

QString ReportQuery::get_table_name_suggestion() const {
    //return QFileInfo(data_engine_source_file_m).fileName().split('.')[0];
    auto sl = data_engine_source_file_m.split(".");
    sl.removeLast();
    // remove file extension ".json"
    return sl.join("/");
}

void ReportQuery::set_table_name(QString table_name) {
    table_name_m = table_name;
}

QString ReportQuery::get_table_name() const {
    assert(table_name_m != "");
    return table_name_m;
}

void ReportQuery::update_from_gui() {
    if (edt_query_report_folder_m) {
        report_path_m = edt_query_report_folder_m->text();
    }
    if (edt_query_data_engine_source_file_m) {
        data_engine_source_file_m = edt_query_data_engine_source_file_m->text();
    }
}

DataEngineSourceFields ReportQuery::get_data_engine_fields_raw() const {
    std::ifstream f(data_engine_source_file_m.toStdString());
    DataEngineSourceFields result{};

    result.general_fields_m = QList<DataEngineField>{
        {"data_source_path", EntryType::Text},        {"datetime_str", EntryType::DateTime},      {"datetime_unix", EntryType::Number},
        {"everything_complete", EntryType::Bool},     {"everything_in_range", EntryType::Bool},   {"exceptional_approval_exists", EntryType::Bool},
        {"framework_git_hash", EntryType::Text},      {"os_username", EntryType::Text},           {"script_path", EntryType::Text},
        {"test_duration_seconds", EntryType::Number}, {"test_git_date_str", EntryType::DateTime}, {"test_git_hash", EntryType::Text},
        {"test_git_modified", EntryType::Text}};
    Data_engine data_engine;
    data_engine.set_source(f);
    auto section_names = data_engine.get_section_names();
    for (auto section_name : section_names) {
        QStringList field_names = data_engine.get_ids_of_section(section_name);
        QList<DataEngineField> field_names_result;
        for (auto field_name : field_names) {
            DataEngineField data_engine_field;
            data_engine_field.field_type_m = data_engine.get_entry_type_dummy_mode(field_name);
            data_engine_field.field_name_m = field_name.split('/')[1];
            field_names_result.append(data_engine_field);
        }
        result.report_fields_m.insert(section_name, field_names_result);
    }
    return result;
}

DataEngineSourceFields ReportQuery::get_data_engine_fields() {
    try {
        DataEngineSourceFields result = get_data_engine_fields_raw();
        is_valid_m = true;
        return result;
    } catch (DataEngineError &e) {
        QMessageBox::warning(MainWindow::mw, QString("Dataengine error"), QString("Dataengine error:\n\n %1").arg(e.what()));
    }
    return DataEngineSourceFields{};
}

void ReportHistoryQuery::on_btn_save_query_clicked() {
    if (QFileInfo::exists(query_filename_m)) {
        load_select_ui_to_query();
        report_query_config_file_m.save_to_file(query_filename_m);
    } else {
        on_btn_save_query_as_clicked();
    }
}

void ReportHistoryQuery::on_btn_save_query_as_clicked() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Report Query file (*.rqu)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDirectory(QSettings{}.value(Globals::report_history_query_path, "").toString());
    if (dialog.exec()) {
        load_select_ui_to_query();
        report_query_config_file_m.save_to_file(dialog.selectedFiles()[0]);
        add_recent_query_files(dialog.selectedFiles()[0]);
    }
}

void ReportHistoryQuery::on_cmb_query_recent_currentIndexChanged(int index) {
    QVariant data = ui->cmb_query_recent->itemData(index, Qt::UserRole);
    if (data.canConvert(QVariant::String)) {
        load_query_from_file(data.toString());
    }
}

void ReportHistoryQuery::on_btn_import_clicked() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Report Query file (*.rqu)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDirectory(QSettings{}.value(Globals::report_history_query_path, "").toString());
    if (dialog.exec()) {
        load_query_from_file(dialog.selectedFiles()[0]);
        add_recent_query_files(dialog.selectedFiles()[0]);
    }
}
void ReportHistoryQuery::load_recent_query_files() {
    ui->cmb_query_recent->clear();
    QStringList recent_query_file_names = QSettings{}.value(Globals::recent_report_history_query_paths, "").toStringList();
    QDir root_dir = QSettings{}.value(Globals::report_history_query_path, "").toString();
    for (auto &s : recent_query_file_names) {
        QString r_p = root_dir.relativeFilePath(s);
        ui->cmb_query_recent->addItem(r_p, QVariant(s));
    }
}

void ReportHistoryQuery::add_recent_query_files(QString file_name) {
    QStringList recent_query_file_names = QSettings{}.value(Globals::recent_report_history_query_paths, "").toStringList();
    if (recent_query_file_names.contains(file_name)) {
        return;
    }
    recent_query_file_names.insert(0, file_name);
    QSettings{}.setValue(Globals::recent_report_history_query_paths, recent_query_file_names);
    load_recent_query_files();
}

void ReportHistoryQuery::reduce_tb_query_path() {
    QStringList sl;
    for (int i = 0; i < ui->tb_queries->count(); i++) {
        sl.append(ui->tb_queries->itemText(i));
    }
    sl = reduce_path(sl);
    for (int i = 0; i < ui->tb_queries->count(); i++) {
        ui->tb_queries->setItemText(i, sl[i]);
    }
    ui->tb_queries->setCurrentIndex(ui->tb_queries->count() - 1);
}

QStringList reduce_path(QStringList sl) {
    // test1/a/b/c/d.json/report/data/sn
    // test1/a/b/c/e.json/report/data/sn
    // test1/a/b/c/e.json/report/data/datetime
    // test1/a/b/c/e.json/report/data/today
    // test1/a/b/c/e.json/report/data2/today
    //
    //will be:
    //
    // d.json/report/data/sn
    // e.json/report/data/sn
    // datetime
    // data/today
    // data2/today
    QStringList result;
    QList<QStringList> tokens_list;
    QList<int> indexes;
    for (auto &s : sl) {
        auto tokens_ = s.split("/");
        tokens_list.append(tokens_);
        QString last_token = tokens_.last();
        result.append(last_token);
        indexes.append(tokens_.count() - 1);
    }
    bool still_duplicates = true;
    while (still_duplicates) {
        still_duplicates = false;
        for (auto &s : result) {
            QList<int> duplicates;
            int index = 0;
            for (auto &f : result) {
                if ((s == f) && (indexes[index] > 0)) {
                    duplicates.append(index);
                }
                index++;
            }
            if (duplicates.count() > 1) {
                still_duplicates = true;
                for (int index : duplicates) {
                    auto &sl = tokens_list[index];
                    int i = indexes[index] - 1;
                    indexes[index] = i;
                    if (i >= 0) {
                        assert(i >= 0);

                        QString prefix = sl[i];
                        result[index] = prefix + "/" + result[index];
                    }
                }
            }
        }
    }
    return result;
}

void ReportDatabase::build_link_tree() {
    QMap<QString, ReportTableLink> receiver_links_all;

    for (const auto &map_pair_outer : tables_m) {
        ReportTable *table_outer = map_pair_outer.second.get();
        const QString &sender_table_name = map_pair_outer.first;
        const ReportTableLink &sender_link = table_outer->get_sender_link();

        int sender_link_key = sender_link.get_field_key_other();

        for (const auto &map_pair_inner : tables_m) {
            ReportTable *table_inner = map_pair_inner.second.get();
            const QString &reciever_table_name = map_pair_inner.first;
            if (table_inner->field_exists(sender_link_key)) {
                ReportTableLink sender_link_reversed(sender_link.reversed());
                sender_link_reversed.set_table_other(table_outer);
                receiver_links_all.insertMulti(reciever_table_name, sender_link_reversed);
                table_outer->set_sender_link_table(table_inner);
            }
        }
    }
    for (auto &table_pair : tables_m) {
        const QString &table_name = table_pair.first;
        ReportTable *table = table_pair.second.get();

        if (receiver_links_all.contains(table_name)) {
            const QList<ReportTableLink> &receiver_link_table = receiver_links_all.values(table_name);
            table->set_receiver_links(receiver_link_table);
        } else {
            QList<ReportTableLink> empty_links;
            table->set_receiver_links(empty_links);
        }
    }
}

void ReportDatabase::join() {
    get_root_table()->integrate_sending_tables();
}

ReportTable *ReportDatabase::get_root_table() {
    //find root:
    //we should start with begin()
    //auto iter = tables_m.end();
    //iter--;
    auto iter = tables_m.begin();
    ReportTable *root_table_probe = iter->second.get();
    ReportTable *root_table = root_table_probe;
    while (root_table_probe) {
        root_table = root_table_probe;
        root_table_probe = root_table_probe->get_sender_link().get_table_other();
    }
    return root_table;
}

void ReportTable::integrate_sending_tables() {
    for (auto &rlink_key : receiver_links_m.uniqueKeys()) {
        for (auto &rlink : receiver_links_m.values(rlink_key)) {
            ReportTable *other_table = rlink.get_table_other();
            if (other_table) {
                other_table->integrate_sending_tables();
            } else {
            }
        }
    }
    if (sender_link_m.table_other_m) {
        sender_link_m.table_other_m->merge(this); // this is leaf
    }
}

void ReportTable::merge(ReportTable *other_table) {
    //other is leaf
    assert(other_table);
    //qDebug() << "other leaf's fields: " << other_table->get_field_names();
    // qDebug() << "my fields: " << get_field_names();
    int this_link_key = other_table->sender_link_m.get_field_key_other();
    int other_link_key = other_table->sender_link_m.get_field_key_this();
    (void)this_link_key;

    ReportTableLink recevier_link = get_receiver_link_by_key_other(other_link_key);
    ReportTableLink recevier_link_tally = get_receiver_link_by_key_other(other_link_key);
    QMap<int, QString> original_field_name_keys_m = field_name_keys_m;

    ///    QList<ReportTableRow> original_rows = rows_m;

    QMap<QVariant, int> original_receiver_index = recevier_link.index_m; //without copy this gets quite complicated and im lazy.

    for (auto &row : rows_m) {
        row.merged_m = false;
    }
    for (ReportTableRow &other_row : other_table->rows_m) {
        const QVariant &indexed_val = other_row.row_m.value(other_link_key);
        assert(indexed_val.isValid());
        QList<int> this_row_indexes = original_receiver_index.values(indexed_val);
        while (this_row_indexes.count()) { // there might be more than one row in this with this value
            int this_row_index = this_row_indexes[0];
            recevier_link_tally.index_m.remove(indexed_val);
            ReportTableRow &this_row = rows_m[this_row_index];

            if (this_row.merged_m) { //this row already treated with merging?
                                     //if yes..
                QList<int> new_this_row_indexes = duplicate_rows(this_row_indexes);
                remove_cols_by_matching(new_this_row_indexes, original_field_name_keys_m); //restore rows with original cols to join the other row with
                this_row_indexes = new_this_row_indexes;
                continue;
            }
#if 0
            QStringList row_str;
            for (const int &col_key : this_row.row_m.keys()) {
                row_str.append(this_row.row_m.value(col_key).toString() + "(" + QString::number(col_key) + ")");
            }
            qDebug() << QString::number(this_row_index) + ":   " + row_str.join("    |    ");
#endif
            for (auto &other_col_key : other_row.row_m.keys()) {
                assert(!this_row.row_m.contains(other_col_key));
                if (other_col_key == other_link_key) {
                    continue;
                }
                this_row.row_m.insert(other_col_key, other_row.row_m.value(other_col_key));
                if (!field_name_keys_m.contains(other_col_key)) {
                    field_name_keys_m.insert(other_col_key, "");
                }
            }
            this_row.merged_m = true;
            this_row_indexes.removeAt(0);
        }
    }

#if 1
    //remove rows which were not in other_table:
    //we could use also row.merged. but i suppose this is faster
    QList<int> indexes_to_remove;
    for (int index_value : recevier_link_tally.index_m) {
        indexes_to_remove.append(index_value);
    }
    remove_indexes_from_list(rows_m, indexes_to_remove);

//qDebug() << "removed: " << recevier_link_tally.index_m;
#endif
}

ReportTable *ReportDatabase::get_table(QString data_engine_source_file) {
    assert(tables_m.count(data_engine_source_file)); //table_m contains data_engine_source_file?
    return tables_m.at(data_engine_source_file).get();
}

ReportTable *ReportDatabase::new_table(const QString table_name, const QString &link_field_name_this, const QString &link_field_name_other) {
    //int link_receiver_key = -1;
    int link_field_key_other = -1;
    if (link_field_name_other != "") {
        link_field_key_other = dictionary_m.get_int_key_by_field_name(link_field_name_other);
    }
    int link_field_key_this = -1;
    if (link_field_name_this != "") {
        link_field_key_this = dictionary_m.get_int_key_by_field_name(link_field_name_this);
    }
    assert((link_field_name_this.startsWith("report") == false) && (link_field_name_this.startsWith("general") == false));
    QList<QPair<QString, int>> receiver_links;
    tables_m.emplace(table_name,
                     std::make_unique<ReportTable>(link_field_name_this, link_field_key_this, link_field_name_other, link_field_key_other, nullptr));
    return tables_m.at(table_name).get();
}

QMap<int, QVariant> ReportDatabase::translate_row_to_int_key(const QMap<QString, QVariant> &row_with_string_names) {
    QMap<int, QVariant> result;
    for (QString str_key : row_with_string_names.keys()) {
        int int_key = dictionary_m.get_int_key_by_field_name(str_key);
        result.insert(int_key, row_with_string_names.value(str_key));
    }
    return result;
}

QString ReportDatabase::get_field_name_by_int_key(int key) const {
    return dictionary_m.get_field_name_by_int_key(key);
}

int ReportDatabase::get_int_key_by_field_name(const QString &field_name) const {
    return dictionary_m.get_int_key_by_field_name_const(field_name);
}

int ReportDatabase::get_int_key_by_field_name_not_const(const QString &field_name) {
    return dictionary_m.get_int_key_by_field_name(field_name);
}

QString ReportFieldNameDictionary::get_field_name_by_int_key(int key) const {
    assert(dictionary_reverse_m.contains(key));
    return dictionary_reverse_m.value(key);
}

int ReportFieldNameDictionary::get_int_key_by_field_name_const(const QString &field_name) const {
    assert(dictionary_m.contains(field_name));
    return dictionary_m.value(field_name);
}

int ReportFieldNameDictionary::get_int_key_by_field_name(const QString &field_name) {
    assert(field_name != "");
    if (dictionary_m.contains(field_name)) {
        return dictionary_m.value(field_name);
    } else {
        return append_new_field_name(field_name);
    }
}

int ReportFieldNameDictionary::append_new_field_name(const QString &field_name) {
    int new_key = increment_key();
    assert(!dictionary_m.contains(field_name));
    assert(!dictionary_reverse_m.contains(new_key));
    dictionary_m.insert(field_name, new_key);
    dictionary_reverse_m.insert(new_key, field_name);
    return new_key;
}

void ReportTable::append_row(const QMap<int, QVariant> &row, const QDateTime &time_stamp) {
    assert(receiver_link_set_m);
    ReportTableRow rrow;
    rrow.time_stamp_m = time_stamp;
    rrow.row_m = row;
    rrow.visible_m = true;
    for (ReportTableLink &receiver_link : receiver_links_m) {
        const QVariant &link_receiver_content = row.value(receiver_link.get_field_key_this());
        receiver_link.index_m.insertMulti(link_receiver_content, rows_m.count());
    }
    for (const auto &field_key : row.keys()) {
        assert(field_name_keys_m.contains(field_key));
    }
    const QVariant &link_sender_content = row.value(sender_link_m.get_field_key_this());
    sender_link_m.index_m.insertMulti(link_sender_content, rows_m.count());
    rows_m.append(rrow);
}

QList<int> ReportTable::duplicate_rows(QList<int> &row_indexes) {
    assert(receiver_link_set_m);
    QList<int> result;
    for (int index : row_indexes) {
        ReportTableRow rrow = rows_m[index];
        rrow.merged_m = false;
        for (ReportTableLink &receiver_link : receiver_links_m) {
            const QVariant &link_receiver_content = rrow.row_m.value(receiver_link.get_field_key_this());
            receiver_link.index_m.insertMulti(link_receiver_content, rows_m.count());
        }

        for (const auto &field_key : rrow.row_m.keys()) {
            assert(field_name_keys_m.contains(field_key));
        }
        const QVariant &link_sender_content = rrow.row_m.value(sender_link_m.get_field_key_this());
        sender_link_m.index_m.insertMulti(link_sender_content, rows_m.count());
        result.append(rows_m.count());
        rows_m.append(rrow);
    }
    return result;
}

void ReportTable::remove_cols_by_matching(const QList<int> &row_indexes, const QMap<int, QString> &allowed_cols) {
    for (int index : row_indexes) {
        for (int col_key : rows_m[index].row_m.uniqueKeys()) {
            if (!allowed_cols.contains(col_key)) {
                rows_m[index].row_m.remove(col_key);
            }
        }
    }
}

const ReportTableLink &ReportTable::get_sender_link() const {
    return sender_link_m;
}

const QMap<int, ReportTableLink> &ReportTable::get_receiver_links() const {
    return receiver_links_m;
}

ReportTableLink &ReportTable::get_receiver_link_by_key_other(int other_key) {
    for (auto &rlink : receiver_links_m) {
        if (rlink.field_key_other_m == other_key) {
            return rlink;
        }
    }
    throw std::runtime_error("ReportTable: can not find receiver link with other key " + QString::number(other_key).toStdString() + "");
}

void ReportTable::set_receiver_links(const QList<ReportTableLink> &receiver_links) {
    for (const auto &rlink : receiver_links) {
        receiver_links_m.insertMulti(rlink.get_field_key_this(), rlink);
    }
    receiver_link_set_m = true;
}

const QList<ReportTableRow> &ReportTable::get_rows() const {
    return rows_m;
}

void ReportTable::set_field_name_keys(const QMap<int, QString> &field_name_keys) {
    field_name_keys_m = field_name_keys;
}

bool ReportTable::field_exists(int field_name_key) const {
    return field_name_keys_m.contains(field_name_key);
}

#if 1
template <typename T>
void remove_indexes_from_list(QList<T> &list, QList<int> indexes) {
    qSort(indexes.begin(), indexes.end(), [](const int &i1, const int &i2) { return i1 < i2; });
    int index_index = 0;
    for (int index : indexes) {
        list.removeAt(index);
        for (int i = index_index; i < indexes.count(); i++) {
            indexes[i] = indexes[i] - 1;
        }
        index_index++;
    }
}

#endif
