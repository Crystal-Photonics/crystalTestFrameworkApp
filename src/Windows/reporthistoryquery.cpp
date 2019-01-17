#include "reporthistoryquery.h"
#include "Windows/mainwindow.h"
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
}

void ReportHistoryQuery::add_new_query_page() {
    QWidget *tool_widget = new QWidget(ui->tb_queries);
    QGridLayout *grid_layout = new QGridLayout(tool_widget);
    ReportQuery &report_query = report_query_config_file_m.add_new_query(tool_widget);
    ui->tb_queries->addItem(tool_widget, "");
    ui->tb_queries->setCurrentIndex(ui->tb_queries->count() - 1);

    connect(report_query.btn_query_report_file_browse, &QToolButton::clicked, [this, report_query](bool checked) {
        (void)checked; //
        const auto selected_dir = QFileDialog::getExistingDirectory(this, QObject::tr("Open report directory"), report_query.edt_query_report_folder->text(),
                                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!selected_dir.isEmpty()) {
            report_query.edt_query_report_folder->setText(selected_dir);
        }
    });

    connect(report_query.btn_query_data_engine_source_file_browse, &QToolButton::clicked, [this, report_query](bool checked) {
        (void)checked; //
        QFileDialog dialog(this);
        dialog.setDirectory(report_query.edt_query_data_engine_source_file->text());
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setNameFilter(tr("Data Engine Input files (*.json)"));
        if (dialog.exec()) {
            report_query.edt_query_data_engine_source_file->setText(dialog.selectedFiles()[0]);
        }
    });

    connect(report_query.btn_query_add, &QToolButton::clicked, [this](bool checked) {
        (void)checked; //
        add_new_query_page();
    });

    connect(report_query.btn_query_del, &QToolButton::clicked, [this, tool_widget](bool checked) {
        (void)checked; //
        remove_query_page(tool_widget);
    });

    connect(report_query.edt_query_data_engine_source_file, &QLineEdit::textChanged, [this, tool_widget](const QString &arg) {
        int index = ui->tb_queries->indexOf(tool_widget);
        ui->tb_queries->setItemText(index, arg);
    });

    (void)grid_layout;
}

void ReportHistoryQuery::on_tree_query_fields_itemDoubleClicked(QTreeWidgetItem *item, int column) {
    (void)column;
    QString field_id = item->data(0, Qt::UserRole).toString();
    if (field_id.count()) {
        QString field_type_str = item->text(1);
        add_new_where_page(field_id, field_type_str);
    }
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

void ReportHistoryQuery::clear_where_pages() {
    while (ui->tb_where->count()) {
        QWidget *widget = ui->tb_where->widget(0);
        delete widget;
        ui->tb_queries->removeItem(0);
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

void ReportHistoryQuery::on_stk_report_history_currentChanged(int arg1) {
    if (arg1 == 1) {
        ui->tree_query_fields->clear();
        //T:/qt/crystalTestFramework/tests/scripts/report_query/data_engine_source_1.json
        const Qt::ItemFlags item_flags_checkable = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        for (auto &query : report_query_config_file_m.get_queries_not_const()) {
            query.update_from_gui();
            QTreeWidgetItem *root_item = new QTreeWidgetItem(QStringList{query.data_engine_source_file});
            ui->tree_query_fields->addTopLevelItem(root_item);

            const DataEngineSourceFields &fields = query.get_data_engine_fields();
            QTreeWidgetItem *root_general_widget = new QTreeWidgetItem(root_item, QStringList{"general"});
            QTreeWidgetItem *root_report_widget = new QTreeWidgetItem(root_item, QStringList{"report"});

            for (const auto &general_field : fields.general_fields) {
                QString typ_name = "";
                QTreeWidgetItem *general_entry =
                    new QTreeWidgetItem(root_general_widget, QStringList{general_field.field_name, general_field.field_type.to_string()});

                general_entry->setCheckState(0, Qt::Unchecked);
                general_entry->setFlags(item_flags_checkable);
                general_entry->setData(0, Qt::UserRole, "general/" + general_field.field_name);
                //  qDebug() << "general/" + general_field.field_name;
            }

            for (const auto &section_name : fields.report_fields.keys()) {
                QTreeWidgetItem *report_section_entry = new QTreeWidgetItem(root_report_widget, QStringList{section_name});
                for (const auto &data_engine_field : fields.report_fields.value(section_name)) {
                    QTreeWidgetItem *report_entry =
                        new QTreeWidgetItem(report_section_entry, QStringList{data_engine_field.field_name, data_engine_field.field_type.to_string()});
                    report_entry->setCheckState(0, Qt::Unchecked);
                    report_entry->setFlags(item_flags_checkable);
                    report_entry->setData(0, Qt::UserRole, "report/" + section_name + "/" + data_engine_field.field_name);
                    //    qDebug() << "report/" + section_name + "/" + data_engine_field.field_name;
                }
            }
        }
    }
}

void ReportHistoryQuery::on_btn_close_clicked() {
    close();
}

void ReportHistoryQuery::add_new_where_page(const QString &field_id, const QString &field_typ) {
    if (field_id == "") {
        return;
    }
    QWidget *tool_widget = new QWidget(ui->tb_where);
    QGridLayout *grid_layout = new QGridLayout(tool_widget);
    auto &report_query = report_query_config_file_m.add_new_where(tool_widget);
    ui->tb_where->addItem(tool_widget, "");
    ui->tb_where->setCurrentIndex(ui->tb_where->count() - 1);
    ui->tb_where->setItemText(ui->tb_where->count() - 1, field_id);

    (void)report_query;
    (void)grid_layout;
    (void)field_typ;
}

ReportQueryWhereField &ReportQueryConfigFile::add_new_where(QWidget *parent) {
    ReportQueryWhereField report_where{};
    if (parent) {
        QGridLayout *gl = dynamic_cast<QGridLayout *>(parent->layout());
        report_where.plainTextEdit = new QPlainTextEdit();
        gl->addWidget(report_where.plainTextEdit, 0, 1);
    }
    query_where_fields_m.append(report_where);
    return query_where_fields_m.last();
}

ReportQuery &ReportQueryConfigFile::add_new_query(QWidget *parent) {
    ReportQuery report_query{};
    if (parent) {
        QGridLayout *gl = dynamic_cast<QGridLayout *>(parent->layout());

        QLabel *lbl_data_engine_source = new QLabel();
        lbl_data_engine_source->setText(QObject::tr("Data engine source file:"));
        gl->addWidget(lbl_data_engine_source, 0, 0);

        report_query.edt_query_data_engine_source_file = new QLineEdit();
        gl->addWidget(report_query.edt_query_data_engine_source_file, 0, 1);

        report_query.btn_query_data_engine_source_file_browse = new QToolButton();
        report_query.btn_query_data_engine_source_file_browse->setText("..");
        gl->addWidget(report_query.btn_query_data_engine_source_file_browse, 0, 2);

        QLabel *lbl_report_path = new QLabel();
        lbl_report_path->setText(QObject::tr("Test report seach folder:"));
        gl->addWidget(lbl_report_path, 1, 0);

        report_query.edt_query_report_folder = new QLineEdit();
        gl->addWidget(report_query.edt_query_report_folder, 1, 1);

        report_query.btn_query_report_file_browse = new QToolButton();
        report_query.btn_query_report_file_browse->setText("..");
        gl->addWidget(report_query.btn_query_report_file_browse, 1, 2);

        QHBoxLayout *layout_add_del = new QHBoxLayout();
        layout_add_del->addStretch(1);
        report_query.btn_query_add = new QToolButton();
        report_query.btn_query_add->setText("+");
        layout_add_del->addWidget(report_query.btn_query_add);

        report_query.btn_query_del = new QToolButton();
        report_query.btn_query_del->setText("-");
        layout_add_del->addWidget(report_query.btn_query_del);
        gl->addLayout(layout_add_del, 2, 1, 2, 1);
        gl->setRowStretch(4, 1);
    }
    report_queries_m.append(report_query);
    return report_queries_m.last();
}

void ReportQueryConfigFile::remove_query(int index) {
    report_queries_m.removeAt(index);
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
        where_field.field_name = js_where_field_obj["field_name"].toString();
        where_field.incremention_selector_expression = js_where_field_obj["incremention_selector_expression"].toString();
        QJsonArray js_conditions = js_where_field_obj["conditions"].toArray();

        for (auto js_condition : js_conditions) {
            ReportQueryWhereFieldValues field_values;
            QJsonObject obj = js_condition.toObject();
            field_values.include_greater_values_till_next_entry = obj["include_greater_values_till_next_entry"].toBool();
            QJsonArray js_values = obj["values"].toArray();
            for (auto value : js_values) {
                field_values.values.append(value.toInt());
            }
            where_field.field_values.append(field_values);
        }
        query_where_fields_m.append(where_field);
    }

    QJsonArray js_queries = doc_obj["queries"].toArray();
    report_queries_m.clear();
    for (const auto js_query : js_queries) {
        ReportQuery rq;
        const auto js_query_obj = js_query.toObject();
        rq.report_path = js_query_obj["report_path"].toString();
        rq.data_engine_source_file = js_query_obj["data_engine_source_file"].toString();
        QJsonArray js_select_field_name = js_query_obj["select_field_names"].toArray();
        for (const auto v : js_select_field_name) {
            rq.select_field_names.append(v.toString());
        }
        report_queries_m.append(rq);
    }
}

const QList<ReportQuery> &ReportQueryConfigFile::get_queries() {
    return report_queries_m;
}

QList<ReportQuery> &ReportQueryConfigFile::get_queries_not_const() {
    return report_queries_m;
}

const QList<ReportQueryWhereField> &ReportQueryConfigFile::get_where_fields() {
    return query_where_fields_m;
}

QMultiMap<QString, QVariant> ReportQueryConfigFile::filter_and_select_reports(const QList<ReportLink> &report_file_list) const {
    QMultiMap<QString, QVariant> result;
    for (const auto &report_link : report_file_list) {
        ReportFile report_file;
        report_file.load_from_file(report_link.report_path_m);
        if (only_successful_reports) {
            if ((report_file.get_field_value("general/everything_complete").toBool() == false) ||
                (report_file.get_field_value("general/everything_in_range").toBool()) == false) {
                continue;
            }
        }
        //    qDebug() << report_link.query.data_engine_source_file;
        QString data_engine_source_file_name = QFileInfo(report_link.query_m.data_engine_source_file).fileName().split('.')[0];

        //  qDebug() << data_engine_source_file_name;
        for (const auto &report_query_where_field : query_where_fields_m) {
            auto value = report_file.get_field_value(report_query_where_field.field_name);
            if (report_query_where_field.matches_value(value)) {
                for (auto select_field_name : report_link.query_m.select_field_names) {
                    result.insertMulti(data_engine_source_file_name + "/" + select_field_name, report_file.get_field_value(select_field_name));
                }
            }
        }
    }
    return result;
}

QList<ReportLink> ReportQueryConfigFile::scan_folder_for_reports(QString base_dir_str) const {
    QList<ReportLink> result;
    for (const auto &query : report_queries_m) {
        QDir base_dir{base_dir_str};
        QString report_search_path = query.report_path;
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
                    return QVariant(QDateTime::fromMSecsSinceEpoch(round(ms_since_epoch)));
                }
            }
        }
    } else {
        QVariant result = js_value_obj[last_field_name_component].toVariant();
        if (result.type() == QVariant::String) {
            QStringList date_formats_to_test{"yyyy:MM:dd hh:mm:ss", "yyyy-MM-dd hh:mm:ss+tz"};
            //"datetime_str": "2018:12:18 09:31:09"
            //"test_git_date_str": "2018-12-14 18:09:26 +0100", ist "2018-12-14 18:09:26 MESZ

            for (const auto &date_format_to_test : date_formats_to_test) {
                auto format = date_format_to_test.split("+");
                auto str_to_test = result.toString();
                bool uses_timezone = format.count() == 2;
                if (uses_timezone) {
                    if (str_to_test.contains(" +")) {
                        auto datetime_tz_split = str_to_test.split(" +");
                        str_to_test = datetime_tz_split[0];

                    } else if (str_to_test.contains(" -")) {
                        auto datetime_tz_split = str_to_test.split(" -");
                        str_to_test = datetime_tz_split[0];
                    }
                }
                QDateTime test_datetime = QDateTime::fromString(str_to_test, format[0]);
                if (test_datetime.isValid()) {
                    result = test_datetime;
                    break;
                }
            }
        }
        return result;
    }
    return QVariant();
}

bool ReportQueryWhereField::matches_value(QVariant value) const {
    for (int i = 0; i < field_values.count(); i++) {
        auto field_value = field_values[i];
        if (field_value.values.contains(value)) {
            return true;
        }

        if (field_value.include_greater_values_till_next_entry) {
            const bool is_num = (field_value.values.first().type() == QVariant::Int) || (field_value.values.first().type() == QVariant::Double);
            const bool is_date = field_value.values.first().type() == QVariant::DateTime;
            if (is_num || is_date) {
                if (i == field_values.count() - 1) { //is last condition?
                    if (value > field_value.values.last()) {
                        return true;
                    }
                } else {
                    if ((value > field_value.values.last()) && (value < field_values[i + 1].values.first())) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void ReportQuery::update_from_gui() {
    if (edt_query_report_folder) {
        report_path = edt_query_report_folder->text();
    }
    if (edt_query_data_engine_source_file) {
        data_engine_source_file = edt_query_data_engine_source_file->text();
    }
}

DataEngineSourceFields ReportQuery::get_data_engine_fields() const {
    std::ifstream f(data_engine_source_file.toStdString());
    DataEngineSourceFields result{};
    //try
    {
        result.general_fields = QList<DataEngineField>{
            {"data_source_path", EntryType::Text},        {"datetime_str", EntryType::DateTime},      {"datetime_unix", EntryType::Number},
            {"everything_complete", EntryType::Bool},     {"everything_in_range", EntryType::Bool},   {"exceptional_approval_exists", EntryType::Bool},
            {"framework_git_hash", EntryType::Text},      {"os_username", EntryType::Text},           {"script_path", EntryType::Text},
            {"test_duration_seconds", EntryType::Number}, {"test_git_date_str", EntryType::DateTime}, {"test_git_hash", EntryType::Text},
            {"test_git_modified", EntryType::Text}};
        Data_engine data_engine;
        data_engine.set_source(f);
        auto section_names = data_engine.get_section_names();
        for (auto section_name : section_names) {
            QStringList field_ids = data_engine.get_ids_of_section(section_name);
            QList<DataEngineField> field_names;
            for (auto field_name : field_ids) {
                DataEngineField data_engine_field;
                data_engine_field.field_type = data_engine.get_entry_type_dummy_mode(field_name);
                data_engine_field.field_name = field_name.split('/')[1];
                field_names.append(data_engine_field);
            }
            result.report_fields.insert(section_name, field_names);
        }
        //  } catch (DataEngineError &e) {
        //QMessageBox::warning(MainWindow::mw, QString("Dataengine error"), QString("Dataengine error:\n\n %1").arg(e.what()));
    }
    return result;
}
