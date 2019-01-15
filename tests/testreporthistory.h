#ifndef TESTREPORTHISTORY_H
#define TESTREPORTHISTORY_H

#include "autotest.h"
#include <QList>
#include <QObject>

class ReportLink;

class TestReportHistory : public QObject {
    Q_OBJECT

    public:
    private slots:

    void load_query_onfig_file();
    void load_report_file1();
    void scan_report_files();
    void test_match_value();
    void scan_filter_report_files();
    void read_report_fields();

    private:
    bool report_link_list_contains_path(QList<ReportLink> report_link_list, QString path);
};

DECLARE_TEST(TestReportHistory)

#endif // TESTREPORTHISTORY_H
