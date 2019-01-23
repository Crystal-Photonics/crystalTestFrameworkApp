#include "testreporthistory.h"
#include "Windows/reporthistoryquery.h"

void TestReportHistory::load_query_onfig_file() {
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_1.json");
    QCOMPARE(query_config_file.get_queries().count(), 1);
    QCOMPARE(query_config_file.get_queries()[0].select_field_names_m.count(), 3);
    QCOMPARE(query_config_file.get_queries()[0].report_path_m, QString("reports1/"));
    QCOMPARE(query_config_file.get_queries()[0].data_engine_source_file_m, QString("F:\\protokolle"));
    QCOMPARE(query_config_file.get_where_fields().count(), 1);
    QCOMPARE(query_config_file.get_where_fields()[0].field_name_m, QString("report/gerate_daten/seriennummer"));
    QCOMPARE(query_config_file.get_where_fields()[0].incremention_selector_expression_, QString("(\\d*)"));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m.count(), 2);
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[0].values_m.count(), 2);
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[0].values_m[0], QVariant(50050));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[0].values_m[1], QVariant(50056));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[0].include_greater_values_till_next_entry_m, true);

    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[1].values_m.count(), 3);
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[1].values_m[0], QVariant(50070));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[1].values_m[1], QVariant(50042));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[1].values_m[2], QVariant(50043));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values_m[1].include_greater_values_till_next_entry_m, false);
}

void TestReportHistory::load_report_file1() {
    ReportFile report_file;
    report_file.load_from_file("../../tests/scripts/report_query/reports1/8110/mcu_guid3/dump-2018_12_18-09_31_09-001.json");
    QCOMPARE(report_file.get_field_value("general/test_duration_seconds"), QVariant(258));
    QCOMPARE(report_file.get_field_value("report/gerate_daten/pcb2_chargennummer"), QVariant("18/35"));
    QCOMPARE(report_file.get_field_value("report/gerate_daten/seriennummer"), QVariant(8110));
    QCOMPARE(report_file.get_field_value("report/gerate_daten/wakeup_possible_taste"), QVariant(true));

    QCOMPARE(report_file.get_field_value("general/datetime_str").value<DataEngineDateTime>().str(), QString("2018-12-18 09:31:09"));
    QCOMPARE(report_file.get_field_value("general/test_git_date_str").value<DataEngineDateTime>().str(), QString("2018-12-14 18:09:26"));
    QCOMPARE(report_file.get_field_value("report/dacadc/datum_today").value<DataEngineDateTime>().str(), QString("2019-01-17 19:21:10"));
}

void TestReportHistory::scan_report_files() {
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_1.json");
    QString base_dir = "../../tests/scripts/report_query";
    auto file_list = query_config_file.scan_folder_for_reports(base_dir);
    QVERIFY(report_link_list_contains_path(file_list, QDir{base_dir}.absoluteFilePath("reports1/8110/mcu_guid3/dump-2018_12_18-09_31_09-001.json")));
    QVERIFY(report_link_list_contains_path(file_list, QDir{base_dir}.absoluteFilePath("reports1/8106/mcu_guid2/dump-2019_01_10-14_04_39-001.json")));
    QVERIFY(report_link_list_contains_path(file_list, QDir{base_dir}.absoluteFilePath("reports1/8106/mcu_guid2/dump-2019_01_10-11_08_18-001.json")));
}

void TestReportHistory::scan_filter_report_files() {
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_2.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    QMap<QString, QList<QVariant>> query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    auto test_duration_seconds = query_result.value("test_1_source_values/general/test_duration_seconds");
    auto pcb2_chargennummer = query_result.value("test_1_source_values/report/gerate_daten/pcb2_chargennummer");
    auto seriennummer = query_result.value("test_1_source_values/report/gerate_daten/seriennummer");
    QCOMPARE(test_duration_seconds.count(), 3);
    QCOMPARE(pcb2_chargennummer.count(), 3);
    QCOMPARE(seriennummer.count(), 3);

    QVERIFY(test_duration_seconds.contains(QVariant(201)));
    QVERIFY(test_duration_seconds.contains(QVariant(206))); //aus reports1\8106\mcu_guid2
    QVERIFY(pcb2_chargennummer.contains(QVariant("18/35")));
    QVERIFY(seriennummer.contains(QVariant(8112)));
    QVERIFY(seriennummer.contains(QVariant(8111)));
    QVERIFY(seriennummer.contains(QVariant(8106)));
    QVERIFY(!seriennummer.contains(QVariant(8110)));
}

void TestReportHistory::test_match_value() {
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_1.json");
    QVERIFY(!query_config_file.get_where_fields()[0].matches_value(0));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50050));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50056));
    QVERIFY(!query_config_file.get_where_fields()[0].matches_value(50051));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50057));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50069));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50070));
    QVERIFY(!query_config_file.get_where_fields()[0].matches_value(50071));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50042));
    QVERIFY(query_config_file.get_where_fields()[0].matches_value(50043));
    QVERIFY(!query_config_file.get_where_fields()[0].matches_value(50044));
}

void TestReportHistory::read_report_fields() {
    ReportQuery report_query{};
    report_query.data_engine_source_file_m = "../../tests/scripts/report_query/data_engine_source_1.json";
    auto data_engine_sections = report_query.get_data_engine_fields_raw();
    QCOMPARE(data_engine_sections.report_fields_m.keys().count(), 3);
    QVERIFY(data_engine_sections.report_fields_m.keys().contains("test_version"));
    QVERIFY(data_engine_sections.report_fields_m.keys().contains("gerate_daten"));
    QVERIFY(data_engine_sections.report_fields_m.keys().contains("dacadc"));
    QCOMPARE(data_engine_sections.report_fields_m.value("dacadc").count(), 3);
    QCOMPARE(data_engine_sections.report_fields_m.value("gerate_daten").count(), 3);
    QCOMPARE(data_engine_sections.report_fields_m.value("test_version").count(), 4);
    QVERIFY(data_engine_field_list_contains_path(data_engine_sections.report_fields_m.value("test_version"), "git_framework"));
    QVERIFY(data_engine_field_list_contains_path(data_engine_sections.report_fields_m.value("gerate_daten"), "pcb1_chargennummer"));
    QVERIFY(data_engine_field_list_contains_path(data_engine_sections.report_fields_m.value("dacadc"), "wakeup_possible"));

    QCOMPARE(get_data_engine_field(data_engine_sections.report_fields_m.value("test_version"), "git_framework").field_type_m.t, EntryType::Text);
    QCOMPARE(get_data_engine_field(data_engine_sections.report_fields_m.value("gerate_daten"), "seriennummer").field_type_m.t, EntryType::Number);
    QCOMPARE(get_data_engine_field(data_engine_sections.report_fields_m.value("dacadc"), "wakeup_possible").field_type_m.t, EntryType::Bool);
    //refernz felder noch zu testen
    auto referenz_liste =
        QStringList{"data_source_path",   "datetime_str", "datetime_unix", "everything_complete",   "everything_in_range", "exceptional_approval_exists",
                    "framework_git_hash", "os_username",  "script_path",   "test_duration_seconds", "test_git_date_str",   "test_git_hash",
                    "test_git_modified"};
    QCOMPARE(data_engine_sections.general_fields_m.count(), referenz_liste.count());
    for (auto ref : referenz_liste) {
        QVERIFY(data_engine_field_list_contains_path(data_engine_sections.general_fields_m, ref));
    }
}

void TestReportHistory::test_reduce_path() {
    QStringList input{
        //
        {"test1/a/b/c/d.json/report/data/sn"}, //
        {"test1/a/b/c/e.json/report/data/sn"},
        {"test1/a/b/c/e.json/report/data/datetime"},
        {"test1/a/b/c/e.json/report/data/today"},
        {"test1/a/b/c/e.json/report/data2/today"},
        {"test1/a/b/c/e.json/report/data/equal/string"},
        {"test1/a/b/c/e.json/report/data/equal/string"}
        //
    };

    QStringList output_wanted{
        //
        {"d.json/report/data/sn"}, //
        {"e.json/report/data/sn"},
        {"datetime"},
        {"data/today"},
        {"data2/today"},
        {"test1/a/b/c/e.json/report/data/equal/string"},
        {"test1/a/b/c/e.json/report/data/equal/string"}
        //
    };
    auto output = ReportHistoryQuery::reduce_path(input);
    int i = 0;
    for (auto &out_s : output) {
        QCOMPARE(out_s, output_wanted[i]);
        i++;
    }
}

bool TestReportHistory::data_engine_field_list_contains_path(const QList<DataEngineField> &data_engine_field_list, QString field_name) {
    for (auto rl : data_engine_field_list) {
        if (rl.field_name_m == field_name) {
            return true;
        }
    }
    return false;
}

const DataEngineField TestReportHistory::get_data_engine_field(const QList<DataEngineField> &data_engine_field_list, QString field_name) {
    DataEngineField result;
    for (auto &rl : data_engine_field_list) {
        if (rl.field_name_m == field_name) {
            return rl;
        }
    }
    return result;
}

bool TestReportHistory::report_link_list_contains_path(const QList<ReportLink> &report_link_list, QString path) {
    for (auto rl : report_link_list) {
        if (rl.report_path_m == path) {
            return true;
        }
    }
    return false;
}
