#include "testreporthistory.h"
#include "windows/reporthistoryquery.h"

void TestReportHistory::load_query_onfig_file() {
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_1.json");
    QCOMPARE(query_config_file.get_queries().count(), 1);
    QCOMPARE(query_config_file.get_queries()[0].select_field_names.count(), 3);
    QCOMPARE(query_config_file.get_queries()[0].report_path, QString("reports1/"));
    QCOMPARE(query_config_file.get_queries()[0].data_engine_source_file, QString("F:\\protokolle"));
    QCOMPARE(query_config_file.get_where_fields().count(), 1);
    QCOMPARE(query_config_file.get_where_fields()[0].field_name, QString("report/gerate_daten/seriennummer"));
    QCOMPARE(query_config_file.get_where_fields()[0].incremention_selector_expression, QString("(\\d*)"));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values.count(), 2);
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[0].values.count(), 2);
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[0].values[0], QVariant(50050));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[0].values[1], QVariant(50056));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[0].include_greater_values_till_next_entry, true);

    QCOMPARE(query_config_file.get_where_fields()[0].field_values[1].values.count(), 3);
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[1].values[0], QVariant(50070));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[1].values[1], QVariant(50042));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[1].values[2], QVariant(50043));
    QCOMPARE(query_config_file.get_where_fields()[0].field_values[1].include_greater_values_till_next_entry, false);
}

void TestReportHistory::load_report_file1() {
    ReportFile report_file;
    report_file.load_from_file("../../tests/scripts/report_query/reports1/8110/mcu_guid3/dump-2018_12_18-09_31_09-001.json");
    QCOMPARE(report_file.get_field_value("general/test_duration_seconds"), QVariant(258));
    QCOMPARE(report_file.get_field_value("report/gerate_daten/pcb2_chargennummer"), QVariant("18/35"));
    QCOMPARE(report_file.get_field_value("report/gerate_daten/seriennummer"), QVariant(8110));
    QCOMPARE(report_file.get_field_value("report/gerate_daten/wakeup_possible_taste"), QVariant(true));
    QCOMPARE(report_file.get_field_value("general/datetime_str"), QVariant(QDateTime::fromString("2018.12.18 09:31:09", "yyyy.MM.dd hh:mm:ss")));
    QCOMPARE(report_file.get_field_value("general/test_git_date_str"), QVariant(QDateTime::fromString("2018.12.14 18:09:26", "yyyy.MM.dd hh:mm:ss")));
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
    QMultiMap<QString, QVariant> query_result = query_config_file.filter_and_select_reports(file_list);
    auto test_duration_seconds = query_result.values("test_1_source_values/general/test_duration_seconds");
    auto pcb2_chargennummer = query_result.values("test_1_source_values/report/gerate_daten/pcb2_chargennummer");
    auto seriennummer = query_result.values("test_1_source_values/report/gerate_daten/seriennummer");
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

bool TestReportHistory::report_link_list_contains_path(QList<ReportLink> report_link_list, QString path) {
    for (auto rl : report_link_list) {
        if (rl.report_path == path) {
            return true;
        }
    }
    return false;
}
