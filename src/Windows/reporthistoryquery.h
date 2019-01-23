#ifndef TESTRESULTQUERY_H
#define TESTRESULTQUERY_H

#include "data_engine/data_engine.h"
#include <QDialog>
#include <QJsonObject>
#include <QMap>
#include <QString>

class QVariant;
class QTreeWidgetItem;
class TestReportHistory;
class QLineEdit;
class QToolButton;
class QGridLayout;
class QPlainTextEdit;
class QProgressDialog;
class QLabel;

namespace Ui {
    class ReportHistoryQuery;
}

//std::variant;

class WhereFieldInterpretationError : public std::runtime_error {
    public:
    WhereFieldInterpretationError(const QString &str)
        : std::runtime_error(str.toStdString()) {}
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

class ReportQueryWhereFieldValues {
    public:
    ReportQueryWhereFieldValues()
        : values_m(QList<QVariant>())
        , include_greater_values_till_next_entry_m(false) {}
    QList<QVariant> values_m;
    bool include_greater_values_till_next_entry_m;
};

class ReportQueryWhereField {
    public:
    bool matches_value(QVariant value) const;

    void load_values_from_plain_text();
    QString field_name_m;
    QString incremention_selector_expression_;
    EntryType field_type_m{EntryType::Unspecified};
    QList<ReportQueryWhereFieldValues> field_values_m;

    QPlainTextEdit *plainTextEdit_m = nullptr;
    QWidget *parent_m = nullptr;
    QLabel *lbl_warning_m = nullptr;
};

class DataEngineField {
    public:
    QString field_name_m;
    EntryType field_type_m{EntryType::Unspecified};
};

class DataEngineSourceFields {
    public:
    QList<DataEngineField> general_fields_m;
    QMap<QString, QList<DataEngineField>> report_fields_m;
};

class ReportQuery {
    friend TestReportHistory;

    public:
    QString report_path_m;
    QString data_engine_source_file_m;
    QStringList select_field_names_m;
    void update_from_gui();
    DataEngineSourceFields get_data_engine_fields();
    QLineEdit *edt_query_report_folder_m = nullptr;
    QLineEdit *edt_query_data_engine_source_file_m = nullptr;
    QToolButton *btn_query_data_engine_source_file_browse_m = nullptr;
    QToolButton *btn_query_report_file_browse_m = nullptr;
    QToolButton *btn_query_add_m = nullptr;
    QToolButton *btn_query_del_m = nullptr;

    protected:
    DataEngineSourceFields get_data_engine_fields_raw() const;

    private:
    bool is_valid_m = false;
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
    ReportQuery &add_new_query(QWidget *parent);
    ReportQuery &find_query(QString data_engine_source_file);
    const QList<ReportQueryWhereField> &get_where_fields();
    QList<ReportQueryWhereField> &get_where_fields_not_const();
    ReportQueryWhereField &add_new_where(QWidget *parent, QString field_name, EntryType field_type);
    bool remove_where(QString field_name);
    void remove_query(int index);
    QMap<QString, QList<QVariant>> execute_query(QWidget *parent) const;

    void create_new_query_ui(QWidget *parent, ReportQuery &report_query);

    void create_new_where_ui(QWidget *parent, ReportQueryWhereField &report_where);

    protected:
    QList<ReportLink> scan_folder_for_reports(QString base_dir_str) const;
    QMap<QString, QList<QVariant>> filter_and_select_reports(const QList<ReportLink> &report_file_list, QProgressDialog *progress_dialog) const;

    private:
    QList<ReportQuery> report_queries_m;
    QList<ReportQueryWhereField> query_where_fields_m;
    QString file_name_m;
    bool only_successful_reports_m = true;

    friend TestReportHistory; //for tests
};

class ReportHistoryQuery : public QDialog {
    Q_OBJECT

    public:
    explicit ReportHistoryQuery(QWidget *parent = nullptr);
    void load_data_engine_source_file(QString file_name);
    void load_query_from_file(QString file_name);
    ~ReportHistoryQuery();
    static QStringList reduce_path(QStringList sl);
    private slots:
    void on_btn_next_clicked();
    void on_btn_back_clicked();
    void on_btn_close_clicked();
    void on_stk_report_history_currentChanged(int arg1);
    void on_tree_query_fields_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_btn_where_del_clicked();
    void on_tree_query_fields_itemClicked(QTreeWidgetItem *item, int column);

    void on_btn_import_clicked();

    void on_cmb_query_recent_currentIndexChanged(int index);

    void on_btn_save_query_clicked();

    void on_btn_save_query_as_clicked();

    void on_toolButton_3_clicked();

    private:
    Ui::ReportHistoryQuery *ui;
    ReportQueryConfigFile report_query_config_file_m;
    void add_new_query_page(ReportQuery &report_query, QGridLayout *grid_layout, QWidget *tool_widget);
    void add_new_query_page();
    void remove_query_page(QWidget *tool_widget);
    void clear_query_pages();
    void add_new_where_page(const QString &field_id, EntryType field_typ);
    void add_new_where_page(ReportQueryWhereField &report_where, QGridLayout *grid_layout, QWidget *tool_widget);

    void clear_where_pages();
    void remove_where_page(int index);
    bool load_select_ui_to_query();
    void load_recent_query_files();
    void add_recent_query_files(QString file_name);
    int old_stk_report_history_index_m = 0;
    QString query_filename_m;
    void reduce_tb_query_path();
};

#endif // TESTRESULTQUERY_H
