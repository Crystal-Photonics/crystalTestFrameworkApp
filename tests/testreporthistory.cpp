#include "testreporthistory.h"
#include "Windows/reporthistoryquery.h"

#define DISABLE_ALL 0

void TestReportHistory::load_query_onfig_file() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_1.json");
    QCOMPARE(query_config_file.get_queries().count(), 1);
    QCOMPARE(query_config_file.get_queries()[0].select_field_names_m.count(), 3);
    QCOMPARE(query_config_file.get_queries()[0].report_path_m, QString("reports1/"));
    QCOMPARE(query_config_file.get_queries()[0].data_engine_source_file_m, QString("data_engine_source_1.json"));
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
#endif
}

void TestReportHistory::load_report_file1() {
#if !DISABLE_ALL || 0
    ReportFile report_file;
    report_file.load_from_file("../../tests/scripts/report_query/reports1/8110/mcu_guid3/dump-2018_12_18-09_31_09-001.json");
    QCOMPARE(report_file.get_field_values("general/test_duration_seconds").first(), QVariant(258));
    QCOMPARE(report_file.get_field_values("report/gerate_daten/pcb2_chargennummer").first(), QVariant("18/35"));
    QCOMPARE(report_file.get_field_values("report/gerate_daten/seriennummer").first(), QVariant(8110));
    QCOMPARE(report_file.get_field_values("report/gerate_daten/wakeup_possible_taste").first(), QVariant(true));

    QCOMPARE(report_file.get_field_values("general/datetime_str").first().value<DataEngineDateTime>().str(), QString("2018-12-18 09:31:09"));
    QCOMPARE(report_file.get_field_values("general/test_git_date_str").first().value<DataEngineDateTime>().str(), QString("2018-12-14 18:09:26"));
    QCOMPARE(report_file.get_field_values("report/dacadc/datum_today").first().value<DataEngineDateTime>().str(), QString("2019-01-17 19:21:10"));
#endif
}

void TestReportHistory::scan_report_files() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_1.json");
    QString base_dir = "../../tests/scripts/report_query";
    auto file_list = query_config_file.scan_folder_for_reports(base_dir);
    QVERIFY(report_link_list_contains_path(file_list, QDir{base_dir}.absoluteFilePath("reports1/8110/mcu_guid3/dump-2018_12_18-09_31_09-001.json")));
    QVERIFY(report_link_list_contains_path(file_list, QDir{base_dir}.absoluteFilePath("reports1/8106/mcu_guid2/dump-2019_01_10-14_04_39-001.json")));
    QVERIFY(report_link_list_contains_path(file_list, QDir{base_dir}.absoluteFilePath("reports1/8106/mcu_guid2/dump-2019_01_10-11_08_18-001.json")));
#endif
}

void TestReportHistory::scan_filter_report_files() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_2.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);

    ReportTable *table_1 = query_result.get_table("data_engine_source_1");
    //qDebug() << *table_1;
    QCOMPARE(table_1->get_rows().count(), 3);
    QMap<int, QStringList> allowed_values1{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), {"206", "201", "211"}},                //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), {"18/35", "18/35", "18/35"}}, //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/seriennummer"), {"8106", "8111", "8112"}}           //
    };
    TestReportHistory::verify_table(table_1, allowed_values1);

    ReportTable *table_2 = query_result.get_table("data_engine_source_2");
    //qDebug() << *table_2;
    QCOMPARE(table_2->get_rows().count(), 2);
    QMap<int, QStringList> allowed_values2{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), {"8106", "8112"}},           //
        {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), {}},                            //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), {"18/33", "18/33"}} //
    };
    TestReportHistory::verify_table(table_2, allowed_values2);

#endif
}

void TestReportHistory::complexer_table_links_1() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_complexer_links_1.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    ReportTable *table_1 = query_result.get_table("data_engine_source_1");
    ReportTable *table_2 = query_result.get_table("data_engine_source_2");
    ReportTable *table_3 = query_result.get_table("data_engine_source_3");
    const QMap<int, ReportTableLink> &rx_links1 = table_1->get_receiver_links();
    const QMap<int, ReportTableLink> &rx_links2 = table_2->get_receiver_links();
    const QMap<int, ReportTableLink> &rx_links3 = table_3->get_receiver_links();
    QCOMPARE(rx_links1.keys().count(), 2);
    QVERIFY(rx_links2.empty());
    QVERIFY(rx_links3.empty());

    QCOMPARE(rx_links1.keys().count(), 2);
    QCOMPARE(rx_links1.uniqueKeys().count(), 1);
    const QList<ReportTableLink> &rxlinks_1 = rx_links1.values(rx_links1.keys()[0]);
    QCOMPARE(rxlinks_1[0].field_name_this_m, QString("data_engine_source_1/report/gerate_daten/seriennummer"));
    QCOMPARE(rxlinks_1[0].field_name_other_m, QString("data_engine_source_2/report/gerate_daten/seriennummer"));
    QCOMPARE(rxlinks_1[1].field_name_this_m, QString("data_engine_source_1/report/gerate_daten/seriennummer"));
    QCOMPARE(rxlinks_1[1].field_name_other_m, QString("data_engine_source_3/report/gerate_daten/seriennummer"));

#endif
}

void TestReportHistory::complexer_table_links_2() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_complexer_links_2.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    ReportTable *table_1 = query_result.get_table("data_engine_source_1");
    ReportTable *table_2 = query_result.get_table("data_engine_source_2");
    ReportTable *table_3 = query_result.get_table("data_engine_source_3");
    ReportTable *table_4 = query_result.get_table("data_engine_source_4");
    const QMap<int, ReportTableLink> &rx_links1 = table_1->get_receiver_links();
    const QMap<int, ReportTableLink> &rx_links2 = table_2->get_receiver_links();
    const QMap<int, ReportTableLink> &rx_links3 = table_3->get_receiver_links();
    const QMap<int, ReportTableLink> &rx_links4 = table_4->get_receiver_links();
    QCOMPARE(rx_links1.keys().count(), 2);
    QCOMPARE(rx_links2.keys().count(), 1);
    QVERIFY(rx_links3.empty());
    QVERIFY(rx_links4.empty());

    QStringList expected_values_1{"data_engine_source_1/report/gerate_daten/seriennummer",
                                  "data_engine_source_1/report/gerate_daten/pcb2_chargennummer"}; //comes from 2 and 3
    QCOMPARE(rx_links1.values(rx_links1.keys()[0]).count(), 1);
    QCOMPARE(rx_links1.values(rx_links1.keys()[1]).count(), 1);
    QVERIFY(expected_values_1.contains(rx_links1.value(rx_links1.keys()[0]).field_name_this_m));
    QVERIFY(expected_values_1.contains(rx_links1.value(rx_links1.keys()[1]).field_name_this_m));

    QCOMPARE(rx_links2.values(rx_links2.keys()[0]).count(), 1);
    QCOMPARE(rx_links2.value(rx_links2.keys()[0]).field_name_this_m, QString("data_engine_source_2/report/gerate_daten/seriennummer")); //comes from 4

#endif
}

void TestReportHistory::join_report_file_tables_empty_result() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_3_empty.json");

    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    query_result.join(false);
    ReportTable *joined_table = query_result.get_root_table();

    // qDebug() << *joined_table;
    // empty

    QCOMPARE(joined_table->get_rows().count(), 0);
#endif
}

void TestReportHistory::join_report_file_tables_1() {
#if !DISABLE_ALL || 0

    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_2.json");

    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    query_result.join(false);
    ReportTable *joined_table = query_result.get_root_table();

    //qDebug() << *joined_table;
    //         |     206(1)     |     18/35(2)     |     8106(3)     |     (5)     |     18/33(6)
    //         |     211(1)     |     18/35(2)     |     8112(3)     |     (5)     |     18/33(6)

    QCOMPARE(joined_table->get_rows().count(), 2);
    QMap<int, QStringList> allowed_values{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/seriennummer"), {"8106", "8112"}},           //
        {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), {"206", "211"}},                //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), {"18/35", "18/35"}},   //
        {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), {}},                            //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), {"18/33", "18/33"}} //
    };
    TestReportHistory::verify_table(joined_table, allowed_values);

#endif
}

void TestReportHistory::join_report_file_tables_2() {
#if !DISABLE_ALL || 0

    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_3_multiple_sn.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    query_result.join(false);
    ReportTable *joined_table = query_result.get_root_table();

#if 0
    QMap<int, QString> field_keys = joined_table->get_field_names();
    for (int key : field_keys.keys()) {
        qDebug() << query_result.get_field_name_by_int_key(key) << "(" + QString::number(key) + ")";
    }
    qDebug() << *joined_table;
#endif
//206(1)     |     18/35(2)     |     8106(3)     |     (5)     |     18/33_8106(6)
//211(1)     |     18/35(2)     |     8112(3)     |     (5)     |     18/33_8112(6)
//206(1)     |     18/35(2)     |     8106(3)     |     (5)     |     18/33(6)

#if 1
    QCOMPARE(joined_table->get_rows().count(), 3);
    QMap<int, QStringList> allowed_values{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/seriennummer"), {"8106", "8106", "8112"}},                      //
        {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), {"206", "206", "211"}},                            //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), {"18/35", "18/35", "18/35"}},             //
        {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), {}},                                               //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), {"18/33_8106", "18/33_8112", "18/33"}} //
    };
    TestReportHistory::verify_table(joined_table, allowed_values);
#endif
#endif
}

void TestReportHistory::join_report_file_tables_only_latest_datasets() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_3_multiple_sn.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    query_result.join(true);
    ReportTable *joined_table = query_result.get_root_table();

#if 0
    QMap<int, QString> field_keys = joined_table->get_field_names();
    for (int key : field_keys.keys()) {
        qDebug() << query_result.get_field_name_by_int_key(key) << "(" + QString::number(key) + ")";
    }
    qDebug() << *joined_table;
#endif

//206(64)     |     18/35(65)     |     8106(66)     |     (68)     |     18/33(69)
//211(64)     |     18/35(65)     |     8112(66)     |     (68)     |     18/33_8112(69)
#if 1
    QCOMPARE(joined_table->get_rows().count(), 2);
    QMap<int, QStringList> allowed_values{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/seriennummer"), {"8106", "8112"}},                //
        {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), {"206", "211"}},                     //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), {"18/35", "18/35"}},        //
        {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), {}},                                 //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), {"18/33", "18/33_8112"}} //
    };
    TestReportHistory::verify_table(joined_table, allowed_values);
#endif
#endif
}

void TestReportHistory::join_report_file_tables_only_latest_datasets2() {
#if !DISABLE_ALL || 0
    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_4_multiple_sn.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    query_result.join(true);
    ReportTable *joined_table = query_result.get_root_table();

#if 1
    QMap<int, QString> field_keys = joined_table->get_field_names();
    for (int key : field_keys.keys()) {
        qDebug() << query_result.get_field_name_by_int_key(key) << "(" + QString::number(key) + ")";
    }
    qDebug() << *joined_table;
#endif

//8106(1)     |     206(3)     |     18/35(4)     |     (5)     |     18/33(6)
//8112(1)     |     211(3)     |     18/35(4)     |     (5)     |     18/33_8112(6)
#if 1
    QCOMPARE(joined_table->get_rows().count(), 2);
    QMap<int, QStringList> allowed_values{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), {"8106", "8112"}},                //
        {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), {"206", "211"}},                     //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), {"18/35", "18/35"}},        //
        {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), {}},                                 //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), {"18/33", "18/33_8112"}} //
    };
    TestReportHistory::verify_table(joined_table, allowed_values);
#endif
#endif
}

void TestReportHistory::join_report_file_tables_3() {
#if !DISABLE_ALL || 0

    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_4_multiple_sn.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
    query_result.join(false);
    ReportTable *joined_table = query_result.get_root_table();
#if 0
    QMap<int, QString> field_keys = joined_table->get_field_names();
    for (int key : field_keys.keys()) {
        qDebug() << query_result.get_field_name_by_int_key(key) << "(" + QString::number(key) + ")";
    }
    qDebug() << *joined_table;
//8106(1)     |     206(3)     |     18/35(4)     |     (5)     |     18/33_8106(6)
//8106(1)     |     206(3)     |     18/35(4)     |     (5)     |     18/33(6)
//8112(1)     |     211(3)     |     18/35(4)     |     (5)     |     18/33_8112(6)
#endif
#if 1
    QCOMPARE(joined_table->get_rows().count(), 3);
    QMap<int, QStringList> allowed_values{
        //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), {"8106", "8106", "8112"}},                      //
        {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), {"206", "206", "211"}},                            //
        {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), {"18/35", "18/35", "18/35"}},             //
        {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), {}},                                               //
        {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), {"18/33_8106", "18/33", "18/33_8112"}} //
    };
    TestReportHistory::verify_table(joined_table, allowed_values);
#endif
#endif
}

void TestReportHistory::join_report_file_tables_4() {
#if !DISABLE_ALL || 0

    ReportQueryConfigFile query_config_file;
    query_config_file.load_from_file("../../tests/scripts/report_query/query_config_5_multiple_sn.json");
    auto file_list = query_config_file.scan_folder_for_reports("../../tests/scripts/report_query");
    ReportDatabase query_result = query_config_file.filter_and_select_reports(file_list, nullptr);
#if 0
    ReportTable *table_1 = query_result.get_table("data_engine_source_1");
    qDebug() << *table_1;
//    8106(5)     |     206(6)     |     18/35(7)
//    8111(5)     |     201(6)     |     18/35(7)
//    8112(5)     |     211(6)     |     18/35(7)
    ReportTable *table_2 = query_result.get_table("data_engine_source_2");
    qDebug() << *table_2;
 //   8106(1)     |     (8)     |     18/33_8106(9)
 //   8106(1)     |     (8)     |     18/33(9)
 //   8112(1)     |     (8)     |     18/33_8112(9)
    ReportTable *table_3 = query_result.get_table("data_engine_source_3");
    qDebug() << *table_3;
 //   8106(2)     |     306(3)     |     (4)
 //   8106(2)     |     206(3)     |     (4)
 //   8112(2)     |     211(3)     |     (4)
#endif
    query_result.join(false);
    ReportTable *joined_table = query_result.get_root_table();
#if 0
    QMap<int, QString> field_keys = joined_table->get_field_names();
    for (int key : field_keys.keys()) {
        qDebug() << query_result.get_field_name_by_int_key(key) << "(" + QString::number(key) + ")";
    }
    qDebug() << *joined_table;
//8106(1)     |     306(3)     |     (4)     |     206(6)     |     18/35(7)     |     (8)     |     18/33_8106(9)
//8106(1)     |     306(3)     |     (4)     |     206(6)     |     18/35(7)     |     (8)     |     18/33(9)
//8112(1)     |     211(3)     |     (4)     |     211(6)     |     18/35(7)     |     (8)     |     18/33_8112(9)
//8106(1)     |     206(3)     |     (4)     |     206(6)     |     18/35(7)     |     (8)     |     18/33(9)
//8106(1)     |     206(3)     |     (4)     |     206(6)     |     18/35(7)     |     (8)     |     18/33_8106(9)
#endif
#if 1

    QCOMPARE(joined_table->get_rows().count(), 5);
    QList<QMap<int, QString>> allowed_values{
        {
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), "8106"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/general/test_duration_seconds"), "306"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/report/gerate_daten/pcb2_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), "206"},
            {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), "18/35"},
            {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), "18/33_8106"},
        },
        {
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), "8106"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/general/test_duration_seconds"), "306"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/report/gerate_daten/pcb2_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), "206"},
            {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), "18/35"},
            {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), "18/33"},
        },
        {
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), "8112"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/general/test_duration_seconds"), "211"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/report/gerate_daten/pcb2_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), "211"},
            {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), "18/35"},
            {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), "18/33_8112"},
        },
        {
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), "8106"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/general/test_duration_seconds"), "206"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/report/gerate_daten/pcb2_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), "206"},
            {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), "18/35"},
            {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), "18/33"},
        },
        {
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/seriennummer"), "8106"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/general/test_duration_seconds"), "206"},
            {query_result.get_int_key_by_field_name("data_engine_source_3/report/gerate_daten/pcb2_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_1/general/test_duration_seconds"), "206"},
            {query_result.get_int_key_by_field_name("data_engine_source_1/report/gerate_daten/pcb2_chargennummer"), "18/35"},
            {query_result.get_int_key_by_field_name("data_engine_source_2/general/display_chargennummer"), ""},
            {query_result.get_int_key_by_field_name("data_engine_source_2/report/gerate_daten/led_pcb_chargennummer"), "18/33_8106"}, //
        },
    };
    TestReportHistory::verify_table_line_by_line(joined_table, allowed_values);
#endif
#endif
}
void TestReportHistory::test_match_value() {
#if !DISABLE_ALL || 0
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
#endif
}

void TestReportHistory::read_report_fields() {
#if !DISABLE_ALL || 0
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
#endif
}

void TestReportHistory::test_reduce_path() {
#if !DISABLE_ALL || 0
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
    auto output = reduce_path(input);
    int i = 0;
    for (auto &out_s : output) {
        QCOMPARE(out_s, output_wanted[i]);
        i++;
    }
#endif
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

void TestReportHistory::verify_table_line_by_line(ReportTable *table_under_test, QList<QMap<int, QString>> &expected_lines) {
    while (expected_lines.count()) {
        auto expected_line = expected_lines[0];

        bool line_found = true;
        for (auto &row : table_under_test->get_rows()) {
            QCOMPARE(row.row_m.uniqueKeys().count(), expected_line.uniqueKeys().count());

            line_found = true;
            QList<int> expected_keys = expected_line.uniqueKeys();
            for (int expected_key : expected_keys) {
                QString col_str;
                QVariant col = row.row_m.value(expected_key);
                if (col.canConvert<DataEngineDateTime>()) {
                    col_str = col.value<DataEngineDateTime>().str();
                } else {
                    col_str = col.toString();
                }
                //qDebug() << col_str;
                // qDebug() << allowed_values.value(allowed_key);
                if (col_str != expected_line.value(expected_key)) {
                    line_found = false;
                    break;
                }
            }
            if (line_found) {
                break;
            }
        }
        QVERIFY(line_found);
        if (line_found) {
            expected_lines.removeFirst();
        } else {
            break;
        }
    }
    QCOMPARE(expected_lines.count(), 0);
}

void TestReportHistory::verify_table(ReportTable *table_under_test, QMap<int, QStringList> &allowed_values) {
    QCOMPARE(table_under_test->get_field_names().count(), allowed_values.uniqueKeys().count());

    for (auto &row : table_under_test->get_rows()) {
        for (int allowed_key : allowed_values.keys()) {
            QString col_str;
            QVariant col = row.row_m.value(allowed_key);
            if (col.canConvert<DataEngineDateTime>()) {
                col_str = col.value<DataEngineDateTime>().str();
            } else {
                col_str = col.toString();
            }
            //qDebug() << col_str;
            // qDebug() << allowed_values.value(allowed_key);
            QStringList allowed_value_per_col = allowed_values.value(allowed_key);
            if (allowed_value_per_col.count() == 0) {
                QCOMPARE(col_str, QString());
            } else {
                QVERIFY(allowed_value_per_col.contains(col_str));
                allowed_value_per_col.removeOne(col_str);
            }
            allowed_values.insert(allowed_key, allowed_value_per_col);
        }
    }
    for (int allowed_key : allowed_values.keys()) {
        QStringList allowed_value_per_col = allowed_values.value(allowed_key);
        QVERIFY(allowed_value_per_col.count() == 0);
        //qDebug() << allowed_values.value(allowed_key);
    }
}
