#include "test_data_engine.h"
#include "data_engine/data_engine.h"

#include <QString>
#include <sstream>

#define QVERIFY_EXCEPTION_THROWN_error_number(expression, error_number)                                                                                        \
    do {                                                                                                                                                       \
        QT_TRY {                                                                                                                                               \
            QT_TRY {                                                                                                                                           \
                expression;                                                                                                                                    \
                QTest::qFail(                                                                                                                                  \
                    "Expected exception of type DataEngineError"                                                                                               \
                    " to be thrown"                                                                                                                            \
                    " but no exception caught",                                                                                                                \
                    __FILE__, __LINE__);                                                                                                                       \
                return;                                                                                                                                        \
            }                                                                                                                                                  \
            QT_CATCH(const DataEngineError &e) {                                                                                                               \
                QCOMPARE(e.get_error_number(), error_number);                                                                                                  \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        QT_CATCH(const std::exception &e) {                                                                                                                    \
            QByteArray msg = QByteArray() + "Expected exception of type DataEngineError to be thrown but std::exception caught with message: " + e.what();     \
            QTest::qFail(msg.constData(), __FILE__, __LINE__);                                                                                                 \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
        QT_CATCH(...) {                                                                                                                                        \
            QTest::qFail(                                                                                                                                      \
                "Expected exception of type DataEngineError"                                                                                                   \
                " to be thrown"                                                                                                                                \
                " but unknown exception caught",                                                                                                               \
                __FILE__, __LINE__);                                                                                                                           \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

QString operator"" _qs(const char *text, std::size_t size) {
    return QString::fromStdString(std::string{text, text + size});
}

struct TestDataTolerance {
    QString in;
    QString expected;
    double desired_value;
    bool should_error;
    DataEngineErrorNumber expected_error;
};

void Test_Data_engine::check_tolerance_parsing_A() {
    std::vector<TestDataTolerance> in_data{{{"1.5", "1000.5 (±1.5)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},        //
                                            {"5%", "1000.5 (±5%)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},          //
                                            {"+-2", "1000.5 (±2)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},          //
                                            {"+5/*", "≤ 1000.5 (+5)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},       //
                                            {"+0/*", "≤ 1000.5", 1000.5, false, DataEngineErrorNumber::no_data_section_found},            //
                                            {"*/-2", "≥ 1000.5 (-2)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},       //
                                            {"*/-0", "≥ 1000.5", 1000.5, false, DataEngineErrorNumber::no_data_section_found},            //
                                            {"+5/-2", "1000.5 (+5/-2)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},     //
                                            {"+5%/-2%", "1000.5 (+5%/-2%)", 1000.5, false, DataEngineErrorNumber::no_data_section_found}, //
                                            {"+5.2%/*", "≤ 1000.5 (+5.2%)", 1000.5, false, DataEngineErrorNumber::no_data_section_found}, //
                                            {"*/-2%", "≥ 1000.5 (-2%)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},     //
                                            {"*", "1000.5 (±∞)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},            //
                                            {"*/*", "1000.5 (±∞)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},          //
                                            {"+*/-*", "1000.5 (±∞)", 1000.5, false, DataEngineErrorNumber::no_data_section_found},        //
                                            {"+5%/-2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                 //
                                            {"+5/-2%", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                 //
                                            {"5/2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                    //
                                            {"+5/2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                   //
                                            {"5/-2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                   //
                                            {"-2.5", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                   //
                                            {"+3.2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                   //
                                            {"*-2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error},                    //
                                            {"-+2", "", 1000.5, true, DataEngineErrorNumber::tolerance_parsing_error}}

    };

    for (auto &data : in_data) {
        NumericTolerance tol;
        qDebug() << data.in;
        if (data.should_error) {
            QVERIFY_EXCEPTION_THROWN_error_number(

                tol.from_string(data.in);, data.expected_error);
        } else {
            tol.from_string(data.in);
            QCOMPARE(tol.to_string(data.desired_value), data.expected);
        }
    }
}

void Test_Data_engine::basic_load_from_config() {
    std::stringstream input{R"({})"};
    Data_engine de{input};
}

void Test_Data_engine::check_properties_of_empty_set() {
    std::stringstream input{R"({})"};
    const Data_engine de{input};
    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
}

void Test_Data_engine::check_no_data_error_A() {
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[

        ]
    }

}
                            )"};
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine{input}, DataEngineErrorNumber::no_data_section_found);
}

void Test_Data_engine::check_no_data_error_B() {
    std::stringstream input{R"(
{
    "supply-voltage":{
    }

}
                            )"};
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine{input}, DataEngineErrorNumber::no_data_section_found);
}

void Test_Data_engine::check_duplicate_name_error_A() {
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    const Data_engine de{input};
    //should throw exception
}

void Test_Data_engine::check_duplicate_name_error_B() {
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    },
    "supply-voltage":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    const Data_engine de{input};
    //should throw exception
}

void Test_Data_engine::check_value_matching_by_name() {
#if 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    const Data_engine de{input};
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 0.9), true);
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 1.0), true);
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 1.441), true);
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 1.442), true);
#endif
}

void Test_Data_engine::check_value_matching_by_name_and_version_A() {
#if 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "legal_versions":"1.0-1.442",
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    const Data_engine de{input};
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 0.9), false);
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 1.0), true);
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 1.441), true);
    QCOMPARE(de.has_desired_value("supply-voltage/voltage", 1.442), false);
#endif
}

void Test_Data_engine::single_numeric_property_test() {
#if 0
	std::stringstream input{R"([{
							"name": "voltage",
							"value": 1000,
                            "tolerance_abs": 100
							}])"};
	Data_engine de{input};
	QVERIFY(!de.is_complete());
	QVERIFY(!de.all_values_in_range());
	QVERIFY(!de.value_in_range("voltage"));
	QCOMPARE(de.get_desired_value("voltage"), 1000.);
	QCOMPARE(de.get_desired_absolute_tolerance("voltage"), 100.);
	QCOMPARE(de.get_desired_relative_tolerance("voltage"), .1);
	QCOMPARE(de.get_desired_minimum("voltage"), 900.);
	QCOMPARE(de.get_desired_maximum("voltage"), 1100.);
	QCOMPARE(de.get_unit("voltage"), ""_qs);
	de.set_actual_number("voltage", 1000.1234);
	QVERIFY(de.is_complete());
	QVERIFY(de.all_values_in_range());
	QVERIFY(de.value_in_range("voltage"));
#endif
}

void Test_Data_engine::multiple_numeric_properties_test() {
#if 0
	std::stringstream input{R"([
							{
							"name": "voltage",
							"value": 1000,
                            "tolerance_abs": 100
							},
							{
							"name": "current",
							"value": 200,
                            "tolerance_abs": 100
							}
							])"};
	Data_engine de{input};
	de.set_actual_number("voltage", 1000.1234);
	QVERIFY(!de.is_complete());
	QVERIFY(!de.all_values_in_range());
	QVERIFY(de.value_in_range("voltage"));
	QVERIFY(!de.value_in_range("current"));
	de.set_actual_number("current", 500.);
	QVERIFY(de.is_complete());
	QVERIFY(!de.all_values_in_range());
	QVERIFY(!de.value_in_range("current"));
#endif
}

void Test_Data_engine::test_text_entry() {
#if 0
	std::stringstream input{R"([{
							"name": "id",
							"value": "DEV123"
							}])"};
	Data_engine de{input};
	QVERIFY(!de.is_complete());
	QVERIFY(!de.all_values_in_range());
	QVERIFY(!de.value_in_range("id"));
	de.set_actual_text("id", "DEV123456zjuthrgfd");
	QVERIFY(de.is_complete());
	QVERIFY(!de.all_values_in_range());
	QVERIFY(!de.value_in_range("id"));
	de.set_actual_text("id", "DEV123");
	QVERIFY(de.is_complete());
	QVERIFY(de.all_values_in_range());
	QVERIFY(de.value_in_range("id"));
#endif
}

void Test_Data_engine::test_preview() {
#if 0
	std::stringstream input{R"([
							{
								"name": "voltage",
								"value": 1000,
                                "tolerance_abs": 100
							},
							{
								"name": "current",
								"value": "[voltage]",
                                "tolerance_abs": 100
							}
							])"};
	int argc = 1;
	char executable[] = "";
	char *executable2 = executable;
	char **argv = &executable2;
	QApplication app(argc, argv);

	Data_engine de{input};
	de.set_actual_number("voltage", 1000.1234);
	//de.set_actual_number("current", 150.);
	de.generate_pdf("test.xml", "test.pdf");
#endif
}
