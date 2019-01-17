#ifndef TESTRESULTQUERY_H
#define TESTRESULTQUERY_H

#include "data_engine/data_engine.h"
#include <QDialog>
#include <QJsonObject>
#include <QMap>

class QVariant;
class QTreeWidgetItem;
class TestReportHistory;
class QLineEdit;
class QToolButton;

namespace Ui {
    class ReportHistoryQuery;
}

//std::variant;

class ReportFile {
    public:
    ReportFile();
    ~ReportFile();
    void load_from_file(QString file_name);
    QVariant get_field_value(QString field_name);

    private:
    QJsonObject js_report_object_m;
};

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

class DataEngineField {
    public:
    QString field_name;
    EntryType field_type{EntryType::Unspecified};
};

class DataEngineSourceFields {
    public:
    QList<DataEngineField> general_fields;
    QMap<QString, QList<DataEngineField>> report_fields;
};

class ReportQuery {
    public:
    QString report_path;
    QString data_engine_source_file;
    QStringList select_field_names;
    void update_from_gui();
    DataEngineSourceFields get_data_engine_fields() const;

    QLineEdit *edt_query_report_folder = nullptr;
    QLineEdit *edt_query_data_engine_source_file = nullptr;
    QToolButton *btn_query_data_engine_source_file_browse = nullptr;
    QToolButton *btn_query_report_file_browse = nullptr;
    QToolButton *btn_query_add = nullptr;
    QToolButton *btn_query_del = nullptr;
};

class ReportLink {
    public:
    ReportLink(QString report_path, const ReportQuery &query)
        : report_path_m{report_path}
        , query_m{query} {}
    QString report_path_m;
    const ReportQuery &query_m;
};

class ReportQueryConfigFile {
    public:
    ReportQueryConfigFile();
    ~ReportQueryConfigFile();
    void load_from_file(QString file_name);
    void save_to_file(QString file_name);
    const QList<ReportQuery> &get_queries();
    QList<ReportQuery> &get_queries_not_const();
    const QList<ReportQueryWhereField> &get_where_fields();

    ReportQuery &add_new_query(QWidget *parent);
    void remove_query(int index);

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

class ReportHistoryQuery : public QDialog {
    Q_OBJECT

    public:
    explicit ReportHistoryQuery(QWidget *parent = nullptr);
    void load_data_engine_source_file(QString file_name);
    ~ReportHistoryQuery();

    private slots:
    void on_btn_next_clicked();

    void on_btn_back_clicked();

    void on_btn_close_clicked();

    void on_stk_report_history_currentChanged(int arg1);

    void on_tree_query_fields_itemDoubleClicked(QTreeWidgetItem *item, int column);

    private:
    Ui::ReportHistoryQuery *ui;
    ReportQueryConfigFile report_query_config_file_m;
    void add_new_where_page();
    void remove_where_page(QWidget *tool_widget);
    void clear_where_pages();
};

#endif // TESTRESULTQUERY_H
