#ifndef TESTRESULTQUERY_H
#define TESTRESULTQUERY_H

#include "data_engine/data_engine.h"
#include <QDebug>
#include <QDialog>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QTableWidgetItem>
class QVariant;
class QTreeWidgetItem;
class TestReportHistory;
class QLineEdit;
class QToolButton;
class QGridLayout;
class QPlainTextEdit;
class QProgressDialog;
class QLabel;
class ReportQuery;

namespace Ui {
    class ReportHistoryQuery;
}

QStringList reduce_path(QStringList sl);

template <typename T>
void remove_indexes_from_list(QList<T> &list, QList<int> indexes);

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

class ReportQueryLink {
    public:
    int other_id = -1;
    ReportQuery *other_report_query = nullptr;
    QString other_field_name;
    QString me_field_name;
};

class ReportQuery {
    friend TestReportHistory;

    public:
    int id_m = 0;
    QString report_path_m;
    QString get_data_engine_source_file() const;
    void set_data_engine_source_file(const QString &data_engine_source_file);
    QStringList select_field_names_m;
    QString get_table_name() const;
    QString get_table_name_suggestion() const;
    void set_table_name(QString table_name);
    void remove_link();
    ReportQueryLink link;
    void update_from_gui();
    DataEngineSourceFields get_data_engine_fields();

    ReportQuery *follow_link_path_to_root();

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
    QString data_engine_source_file_m;
    QString table_name_m;
    ReportQuery *follow_link_path_to_root_recursion(ReportQuery *other);
};

class ReportLink {
    public:
    ReportLink(QString report_path, const ReportQuery &query)
        : report_path_m{report_path}
        , query_m{query} {}
    QString report_path_m;
    const ReportQuery &query_m;
};

#if 0
struct QueryResult {
    public:
    QMap<QString, QMap<QString, QList<QVariant>>> data;
    QStringList field_names;
};
#endif

class ReportFieldNameDictionary {
    public:
    ReportFieldNameDictionary() {}
    QString get_field_name_by_int_key(int key) const;
    int get_int_key_by_field_name(const QString &field_name);
    int get_int_key_by_field_name_const(const QString &field_name) const;

    private:
    int append_new_field_name(const QString &field_name); //adds table_name/field_section/field_name to dictionary
    QMap<QString, int> dictionary_m;                      //stores the coloum-names and points to ReportTableRow.row_m
    QMap<int, QString> dictionary_reverse_m;              //stores the coloum-names and points to ReportTableRow.row_m

    static int increment_key() {
        static int incrementing_key = 0;
        incrementing_key++;
        return incrementing_key;
    }
};

class ReportTableRow {
    public:
    QMap<int, QVariant> row_m;
    QDateTime time_stamp_m;
    bool visible_m = true;
    bool merged_m = false;
};
class ReportTable;

class ReportTableLink {
    friend ReportTable;

    public:
    ReportTableLink(const QString &field_name_this, const int field_key_this, ReportTable *table_this, const QString &field_name_other,
                    const int field_key_other, ReportTable *table_other)
        : // index_m()
        field_name_this_m(field_name_this)
        , field_name_other_m(field_name_other)
        , field_key_this_m(field_key_this)
        , field_key_other_m(field_key_other)
        , table_this_m(table_this)
        , table_other_m(table_other) {}

    ReportTableLink()
        : field_key_this_m(-1) {}

    ReportTableLink reversed() const {
        return ReportTableLink(field_name_other_m, field_key_other_m, table_other_m, field_name_this_m, field_key_this_m, table_this_m);
    }

    int get_field_key_this() const {
        return field_key_this_m;
    }

    int get_field_key_other() const {
        return field_key_other_m;
    }

    ReportTable *get_table_other() const {
        return table_other_m;
    }

    void set_table_other(ReportTable *table_other) {
        table_other_m = table_other;
    }

    QString field_name_this_m;  //field names are table_name/field_section/field_name
    QString field_name_other_m; //field names are table_name/field_section/field_name
    private:
    int field_key_this_m;  //field names are table_name/field_section/field_name
    int field_key_other_m; //field names are table_name/field_section/field_name

    ReportTable *table_this_m;

    protected:
    ReportTable *table_other_m;
};
class ReportDatabase;
class ReportTable {
    friend ReportDatabase;
    friend TestReportHistory;

    public:
    ReportTable(const QString &link_sender_this_field_name, int link_sender_this_field_key, const QString &link_sender_other_field_name,
                int link_sender_other_field_key, ReportTable *link_sender_other_table)
        : sender_link_m(link_sender_this_field_name,  //
                        link_sender_this_field_key,   //
                        this,                         //
                        link_sender_other_field_name, //
                        link_sender_other_field_key,  //
                        link_sender_other_table) {}

    void append_row(const QMap<int, QVariant> &row, const QDateTime &time_stamp); //appends row and updates index
    void set_field_name_keys(const QMap<int, QString> &field_name_keys);
    const QList<ReportTableRow> &get_rows() const;

    QMap<int, QString> get_field_names() const {
        return field_name_keys_m;
    }

    friend QDebug operator<<(QDebug stream, const ReportTable &table) {
        QStringList header;
        for (int &col_key : table.field_name_keys_m.uniqueKeys()) {
            header.append(table.field_name_keys_m.value(col_key) + "(" + QString::number(col_key) + ")");
        }
        //header = reduce_path(header);
        stream.noquote() << "\n";
        stream.noquote() << header.join("     |     ") + "\n";

        for (auto const &row : table.rows_m) {
            QStringList row_str;
            for (int &col_key : table.field_name_keys_m.uniqueKeys()) {
                auto &col = row.row_m.value(col_key);
                QString col_str;
                if (col.canConvert<DataEngineDateTime>()) {
                    col_str = col.value<DataEngineDateTime>().str();
                } else {
                    col_str = col.toString();
                }
                row_str.append(col_str + "(" + QString::number(col_key) + ")");
            }
            stream.noquote() << row_str.join("     |     ") + "\n";
        }
        return stream;
    }

    protected:
    void set_receiver_links(const QList<ReportTableLink> &receiver_links_m);
    QList<ReportTableRow> get_rows_by_receiver_index(const QVariant &to_be_linked_with_receiver_index); //gets a row which matches to the receiver_link

    void set_sender_link_table(ReportTable *sender_link_table) {
        sender_link_m.table_other_m = sender_link_table;
    }

    bool field_exists(int field_name_key) const;
    void integrate_sending_tables();
    const QMap<int, ReportTableLink> &get_receiver_links() const;

    private:
    void insert_receiver_index_value(const QMap<int, QVariant> &row, int row_index);
    QMap<int, QMap<QVariant, int>>
        receiver_indexes_m; //first key points to this_link_field_key and second key is the content of the field. the final value is the row of the table
    QList<int> duplicate_rows(QList<int> &row_indexes); //clones row and updates index
    const ReportTableLink &get_sender_link() const;
    void remove_cols_by_matching(const QList<int> &row_indexes, const QMap<int, QString> &allowed_cols);
    ReportTableLink &get_receiver_link_by_key_other(int other_key);
    void merge(ReportTable *other_table);
    ReportTableLink sender_link_m;
    QMap<int, ReportTableLink> receiver_links_m; //key == field key, the same as ReportTableLink.field_key_this_m
                                                 //receiver links point to this table

    QList<ReportTableRow> rows_m;
    QMap<int, QString> field_name_keys_m; //fields which could possibly exist
    bool receiver_link_set_m = false;
};

class ReportDatabase {
    public:
    ReportDatabase() {}

    void build_link_tree();
    void join();
    ReportTable *get_root_table();
    ReportTable *get_table(QString data_engine_source_file);
    ReportTable *new_table(const QString table_name, const QString &link_field_name_this, const QString &link_field_name_other);

    QMap<int, QVariant> translate_row_to_int_key(const QMap<QString, QVariant> &row_with_string_names);

    QString get_field_name_by_int_key(int key) const;
    int get_int_key_by_field_name(const QString &field_name) const;
    int get_int_key_by_field_name_not_const(const QString &field_name);

    private:
    ReportFieldNameDictionary dictionary_m;
    std::map<QString, std::unique_ptr<ReportTable>> tables_m; //key is the table name. eg. data_engine_source.json
};

class ReportQueryConfigFile {
    public:
    ReportQueryConfigFile();
    ~ReportQueryConfigFile();
    void load_from_file(QString file_name);
    void save_to_file(QString file_name);
    const QList<ReportQuery> &get_queries() const;
    QList<ReportQuery> &get_queries_not_const();
    ReportQuery &add_new_query(QWidget *parent);
    void build_query_link_references();
    const QList<ReportQueryWhereField> &get_where_fields();
    QList<ReportQueryWhereField> &get_where_fields_not_const();
    ReportQueryWhereField &add_new_where(QWidget *parent, QString field_name, EntryType field_type);
    bool remove_where(QString field_name);
    void remove_query(int index);
    ReportDatabase execute_query(QWidget *parent) const;

    void create_new_query_ui(QWidget *parent, ReportQuery &report_query);

    void create_new_where_ui(QWidget *parent, ReportQueryWhereField &report_where);

    ReportQuery &find_query_by_source_file(QString data_engine_source_file);
    ReportQuery &find_query_by_table_name(QString table_name);
    ReportQuery &find_query_by_id(int id);

    void set_table_names();
    void find_queries_with_other_ids(int other_id, QList<int> &found_ids, bool allow_recursion);
    bool test_links(QString &message);

    protected:
    QList<ReportLink> scan_folder_for_reports(const QString &base_dir_str) const;
    ReportDatabase filter_and_select_reports(const QList<ReportLink> &report_file_list, QProgressDialog *progress_dialog) const;

    private:
    QList<ReportQuery> report_queries_m;
    QList<ReportQueryWhereField> query_where_fields_m;
    QString file_name_m;
    bool only_successful_reports_m = true;
    bool reference_links_built_m = false;

    friend TestReportHistory; //for tests
};

class ReportHistoryQuery : public QDialog {
    Q_OBJECT

    public:
    explicit ReportHistoryQuery(QWidget *parent = nullptr);
    void load_data_engine_source_file(QString file_name);
    void load_query_from_file(QString file_name);
    ~ReportHistoryQuery();

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

    void on_btn_result_export_clicked();

    void link_menu(const QPoint &pos);

    void on_btn_link_field_to_clicked();

    void on_tree_query_fields_itemSelectionChanged();

    private:
    Ui::ReportHistoryQuery *ui;
    ReportQueryConfigFile report_query_config_file_m;
    void add_new_query_page(ReportQuery &report_query, QGridLayout *grid_layout, QWidget *tool_widget);
    void add_new_query_page();
    void remove_query_page(QWidget *tool_widget);
    void clear_query_pages();
    void add_new_where_page(const QString &field_id, EntryType field_typ);
    void add_new_where_page(ReportQueryWhereField &report_where, QGridLayout *grid_layout, QWidget *tool_widget);

    void diplay_links_in_selects();
    void clear_where_pages();
    void remove_where_page(int index);
    bool load_select_ui_to_query();
    void load_recent_query_files();
    void add_recent_query_files(QString file_name);
    int old_stk_report_history_index_m = 0;
    QString query_filename_m;
    void reduce_tb_query_path();
    QTreeWidgetItem *find_widget_by_field_name(int query_id, const QString &field_name);
    QString get_table_name(const QTreeWidgetItem *item);
    int get_table_id(const QTreeWidgetItem *item);
    void create_link_menu(const QPoint &global_pos, QTreeWidgetItem *clicked_item);
};

class MyTableWidgetItem : public QTableWidgetItem {
    public:
    MyTableWidgetItem(const QString &text, int type = Type)
        : QTableWidgetItem(text, type) {}
    bool operator<(const QTableWidgetItem &other) const {
        const QVariant &my_dt = data(Qt::UserRole);
        const QVariant &other_dt = other.data(Qt::UserRole);
        if (my_dt.type() == other_dt.type()) {
            if (my_dt.type() == QVariant::Int) {
                return my_dt.toInt() < other_dt.toInt();
            } else if (my_dt.type() == QVariant::Double) {
                return my_dt.toDouble() < other_dt.toDouble();
            } else if (my_dt.canConvert<DataEngineDateTime>()) {
                return my_dt.value<DataEngineDateTime>().dt() < other_dt.value<DataEngineDateTime>().dt();
            }
        }
        return text().toInt() < other.text().toInt();
    }
};

#endif // TESTRESULTQUERY_H
