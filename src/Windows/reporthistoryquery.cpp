#include "reporthistoryquery.h"
#include "data_engine/data_engine.h"
#include "ui_reporthistoryquery.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>

ReportHistoryQuery::ReportHistoryQuery(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReportHistoryQuery) {
    ui->setupUi(this);
}

ReportHistoryQuery::~ReportHistoryQuery() {
    delete ui;
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

const QList<ReportQueryWhereField> &ReportQueryConfigFile::get_where_fields() {
    return query_where_fields_m;
}

QMultiMap<QString, QVariant> ReportQueryConfigFile::filter_and_select_reports(const QList<ReportLink> &report_file_list) const {
    QMultiMap<QString, QVariant> result;
    for (const auto &report_link : report_file_list) {
        ReportFile report_file;
        report_file.load_from_file(report_link.report_path);
        if (only_successful_reports) {
            if ((report_file.get_field_value("general/everything_complete").toBool() == false) ||
                (report_file.get_field_value("general/everything_in_range").toBool()) == false) {
                continue;
            }
        }
        //    qDebug() << report_link.query.data_engine_source_file;
        QString data_engine_source_file_name = QFileInfo(report_link.query.data_engine_source_file).fileName().split('.')[0];

        //  qDebug() << data_engine_source_file_name;
        for (const auto &report_query_where_field : query_where_fields_m) {
            auto value = report_file.get_field_value(report_query_where_field.field_name);
            if (report_query_where_field.matches_value(value)) {
                for (auto select_field_name : report_link.query.select_field_names) {
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
                if (!js_numeric_obj.isEmpty()) {
                    return QVariant(js_numeric_obj["actual"].toObject()["value"].toDouble());
                } else if (!js_text_obj.isEmpty()) {
                    return QVariant(js_text_obj["actual"].toObject()["value"].toString());
                } else if (!js_bool_obj.isEmpty()) {
                    return QVariant(js_bool_obj["actual"].toObject()["value"].toBool());
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
