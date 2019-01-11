#ifndef TESTRESULTQUERY_H
#define TESTRESULTQUERY_H

#include <QDialog>
#include <QJsonObject>

class QVariant;

class TestReportHistory;

namespace Ui {
    class ReportHistoryQuery;
}

//std::variant;

class ReportQueryWhereFieldValues {
    public:
    QList<QVariant> values;
    bool include_greater_values_till_next_entry;
};

class ReportQueryWhereField {
    public:
    bool matches_value(QVariant value) const;

    QString field_name;
    QString incremention_selector_expression;
    QList<ReportQueryWhereFieldValues> field_values;
};

class ReportQuery {
    public:
    QString report_path;
    QString data_engine_source_file;
    QStringList select_field_names;
};

class ReportLink {
    public:
    QString report_path;
    const ReportQuery &query;
};

class ReportQueryConfigFile {
    public:
    ReportQueryConfigFile();
    ~ReportQueryConfigFile();
    void load_from_file(QString file_name);
    void save_to_file(QString file_name);
    const QList<ReportQuery> &get_queries();
    const QList<ReportQueryWhereField> &get_where_fields();

    protected:
    QList<ReportLink> scan_folder_for_reports(QString base_dir_str) const;
    QMultiMap<QString, QVariant> filter_and_select_reports(const QList<ReportLink> &report_file_list) const;

    private:
    QList<ReportQuery> report_queries_m;
    QList<ReportQueryWhereField> query_where_fields_m;
    QString file_name_m;
    bool only_successful_reports = true;

    friend TestReportHistory; //for tests
};

class ReportFile {
    public:
    ReportFile();
    ~ReportFile();
    void load_from_file(QString file_name);
    QVariant get_field_value(QString field_name);

    private:
    QJsonObject js_report_object_m;
};

class ReportHistoryQuery : public QDialog {
    Q_OBJECT

    public:
    explicit ReportHistoryQuery(QWidget *parent = nullptr);
    void load_data_engine_source_file(QString file_name);
    ~ReportHistoryQuery();

    private:
    Ui::ReportHistoryQuery *ui;
};

#endif // TESTRESULTQUERY_H
