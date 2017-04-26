#include "test_data_engine.h"
#include "data_engine/data_engine.h"

#include <QList>
#include <QPair>
#include <QString>
#include <sstream>

#define DISABLE_ALL 1

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

struct TestDataVersionString {
    QString in;

    QVariant version_number_match_exactly;
    float version_number_low_including;
    float version_number_high_excluding;
    DependencyValue::Match_style match_style;

    bool should_error;
    DataEngineErrorNumber expected_error;
};

void Test_Data_engine::check_version_string_parsing() {
//["1.17-1.42","1.09","1.091"]
//["1.12-1.17","1.06-1.09"]
//"1.12-1.17"
//"1.17"
//1.17
#if !DISABLE_ALL || 0
    std::vector<TestDataVersionString> in_data{{
        {"[1.17-1.42]", 0, 1.17, 1.42, DependencyValue::MatchByRange, false, DataEngineErrorNumber::invalid_version_dependency_string},            //
        {"", "", 0, 0, DependencyValue::MatchNone, false, DataEngineErrorNumber::invalid_version_dependency_string},                               //
        {"[*]", 0, 0, 0, DependencyValue::MatchEverything, false, DataEngineErrorNumber::invalid_version_dependency_string},                       //
        {"[1.17]", 1.17, 0, 0, DependencyValue::MatchExactly, false, DataEngineErrorNumber::invalid_version_dependency_string},                    //
        {"[sdfsdf]", 1.17, 0, 0, DependencyValue::MatchExactly, true, DataEngineErrorNumber::invalid_version_dependency_string},                   //
        {"[1.17-1.42-1.85]", 1.17, 0, 0, DependencyValue::MatchExactly, true, DataEngineErrorNumber::invalid_version_dependency_string},           //
        {"1.17", 1.17, 0, 0, DependencyValue::MatchExactly, false, DataEngineErrorNumber::invalid_version_dependency_string},                      //
        {"sdfsdf", "sdfsdf", 0, 0, DependencyValue::MatchExactly, false, DataEngineErrorNumber::invalid_version_dependency_string},                //
        {"1.17-1.42-1.85", "1.17-1.42-1.85", 0, 0, DependencyValue::MatchExactly, false, DataEngineErrorNumber::invalid_version_dependency_string} //
    }

    };

    for (auto &data : in_data) {
        DependencyValue vers;
        //qDebug() << data.in;
        if (data.should_error) {
            QVERIFY_EXCEPTION_THROWN_error_number(

                vers.from_string(data.in);, data.expected_error);
        } else {
            vers.from_string(data.in);
            if (vers.match_exactly.canConvert<double>()) {
                QCOMPARE(std::round(vers.match_exactly.toDouble() * 1000.0), std::round(data.version_number_match_exactly.toDouble() * 1000.0));
            } else {
                QCOMPARE(vers.match_exactly, data.version_number_match_exactly);
            }
            QCOMPARE(vers.range_low_including, data.version_number_low_including);
            QCOMPARE(vers.range_high_excluding, data.version_number_high_excluding);
            QCOMPARE(vers.match_style, data.match_style);
        }
    }
#endif
}

struct TestDataTolerance {
    QString in;
    QString expected;
    double desired_value;
    bool should_error;
    DataEngineErrorNumber expected_error;
};
void Test_Data_engine::check_tolerance_parsing_A() {
#if !DISABLE_ALL
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
        //qDebug() << data.in;
        if (data.should_error) {
            QVERIFY_EXCEPTION_THROWN_error_number(

                tol.from_string(data.in);, data.expected_error);
        } else {
            tol.from_string(data.in);
            QCOMPARE(tol.to_string(data.desired_value), data.expected);
        }
    }
#endif
}

void Test_Data_engine::basic_load_from_config() {
#if !DISABLE_ALL
    std::stringstream input{R"({})"};
    Data_engine de{input};
#endif
}

void Test_Data_engine::check_properties_of_empty_set() {
#if !DISABLE_ALL
    std::stringstream input{R"({})"};
    const Data_engine de{input};
    QMap<QString, QVariant> tags;
    QVERIFY(de.is_complete(tags));
    QVERIFY(de.all_values_in_range(tags));
#endif
}

void Test_Data_engine::check_no_data_error_A() {
#if !DISABLE_ALL
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
#endif
}

void Test_Data_engine::check_no_data_error_B() {
#if !DISABLE_ALL
    std::stringstream input{R"(
{
    "supply-voltage":{
    }

}
                            )"};
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine{input}, DataEngineErrorNumber::no_data_section_found);
#endif
}

void Test_Data_engine::check_wrong_tolerance_type_A() {
#if !DISABLE_ALL
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": true,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine{input}, DataEngineErrorNumber::tolerance_parsing_error);
#endif
}

void Test_Data_engine::check_duplicate_name_error_A() {
#if !DISABLE_ALL
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
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine{input}, DataEngineErrorNumber::duplicate_field);
#endif
}

void Test_Data_engine::check_duplicate_name_error_B() {
//unfortunatly QJsonDocument doesnt check for duplicate keys
#if !DISABLE_ALL && 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage_a",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    },
    "supply-voltage":{
        "data":[
            {	"name": "voltage_b",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine{input}, DataEngineErrorNumber::duplicate_section);
#endif
}

void Test_Data_engine::check_non_faulty_field_id() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage_test",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};

    Data_engine de{input};
    QMap<QString, QVariant> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value("supply-voltage", tags), DataEngineErrorNumber::faulty_field_id);

#endif
}

void Test_Data_engine::check_non_existing_desired_value() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":[
            {	"name": "voltage_test",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};

    Data_engine de{input};
    QMap<QString, QVariant> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value("supply-voltage/voltage", tags), DataEngineErrorNumber::no_field_id_found);

#endif
}

struct TestDependencyData {
    QList<QPair<QString, QVariant>> values;

    bool should_error;
    DataEngineErrorNumber expected_error;
};

void Test_Data_engine::check_dependency_handling() {
#if !DISABLE_ALL || 0
    std::vector<TestDependencyData> in_data{

        {{{"sound", 1.2}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found}, //
        {{{"sound", 1.42}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.5}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.8}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", false}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "6V"}, {"high_power_edidtion", false}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "12V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.442}, {"main", 1.42}, {"power_supply", 1.5}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found},
        {{{"sound", 1.442}, {"main", 1.42}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found}

    };

    for (const auto &data : in_data) {
        std::stringstream input{R"(
    {
        "supply-voltage":{
            "apply_if":{
                "sound":"[1.42-1.442]",
                "main":"[1.42-1.442]",
                "power_supply":["[1.42-1.442]","[1.5-1.6]","1.8",1.9],
                "color":"[*]",
                "supply_version":["24V","6V"],
                "high_power_edidtion":true
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    }
                                )"};

        Data_engine de{input};
        QMap<QString, QVariant> tags;
        QString printer;
        for (const auto &value : data.values) {
            tags.insert(value.first, value.second);
            printer += value.first + ": " + value.second.toString() + ", ";
        }
        //qDebug() << printer;
        if (data.should_error) {
            QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value("supply-voltage/voltage", tags), DataEngineErrorNumber::non_unique_desired_field_found);
        } else {
            de.get_desired_value("supply-voltage/voltage", tags);
        }
    }
#endif
}

void Test_Data_engine::check_dependency_ambiguity_handling() {
#if !DISABLE_ALL || 0
    std::vector<TestDependencyData> in_data{

        {{{"sound", 1.2}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found}, //
        {{{"sound", 1.42}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found}, //
        {{{"sound", 1.43}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         true,
         DataEngineErrorNumber::non_unique_desired_field_found}, //
        {{{"sound", 1.442}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found}, //
        {{{"sound", 1.5}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         false,
         DataEngineErrorNumber::non_unique_desired_field_found}, //
    };

    for (const auto &data : in_data) {
        std::stringstream input{R"(
    {
        "supply-voltage":[
            {
                "apply_if":{
                    "sound":"[1.42-1.442]",
                    "main":"[1.42-1.442]",
                    "power_supply":["[1.42-1.442]","[1.5-1.6]","1.8",1.9],
                    "color":"[*]",
                    "supply_version":["24V","6V"],
                    "high_power_edidtion":true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                ]
            },
            {
                "apply_if":{
                    "sound":"[1.43-*]",
                    "main":"[1.42-1.442]",
                    "power_supply":["[1.42-1.442]","[1.5-1.6]","1.8",1.9],
                    "color":"[*]",
                    "supply_version":["24V","6V"],
                    "high_power_edidtion":true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                ]
            }
        ]
    }
                                )"};

        Data_engine de{input};
        QMap<QString, QVariant> tags;
        QString printer;
        for (const auto &value : data.values) {
            tags.insert(value.first, value.second);
            printer += value.first + ": " + value.second.toString() + ", ";
        }
        //qDebug() << printer;
        if (data.should_error) {
            QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value("supply-voltage/voltage", tags), DataEngineErrorNumber::non_unique_desired_field_found);
        } else {
            de.get_desired_value("supply-voltage/voltage", tags);
        }
    }
#endif
}

void Test_Data_engine::check_non_existing_section_name() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{
    "supply-voltage_test":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};

    Data_engine de{input};
    QMap<QString, QVariant> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value("supply-voltage/voltage", tags), DataEngineErrorNumber::no_section_id_found);

#endif
}

void Test_Data_engine::check_data_by_object() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "data":
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
    }
}
                            )"};
    Data_engine de{input};
    QMap<QString, QVariant> tags;
    de.get_desired_value("supply-voltage/voltage", tags);

#endif
}
void Test_Data_engine::check_value_matching_by_name() {
#if !DISABLE_ALL || 0
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
    QMap<QString, QVariant> tags;
    QCOMPARE(de.get_desired_value("supply-voltage/voltage", tags), 5000.0);
    QCOMPARE(de.get_desired_value_as_text("supply-voltage/voltage", tags), QString("5000 (±200)"));
#endif
}

void Test_Data_engine::single_numeric_property_test() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "supply":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "current",	 	"value": 100,	"tolerance": "5%",	"unit": "mA", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "power",	 	"value": 100,	"tolerance": "+4/*",	"unit": "mW", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};

    Data_engine de{input};
    QMap<QString, QVariant> tags;
    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("supply/voltage", tags));

    QCOMPARE(de.get_desired_value("supply/voltage", tags), 5000.);
    QCOMPARE(de.get_unit("supply/voltage", tags), "mV"_qs);

    de.set_actual_number("supply/voltage", tags, 5201);
    QVERIFY(!de.value_in_range("supply/voltage", tags));

    de.set_actual_number("supply/voltage", tags, 5200);
    QVERIFY(de.value_in_range("supply/voltage", tags));

    de.set_actual_number("supply/current", tags, 106);
    QVERIFY(!de.value_in_range("supply/current", tags));

    de.set_actual_number("supply/current", tags, 105);
    QVERIFY(de.value_in_range("supply/current", tags));

    de.set_actual_number("supply/power", tags, 105);
    QVERIFY(!de.value_in_range("supply/power", tags));

    de.set_actual_number("supply/power", tags, 104);
    QVERIFY(de.value_in_range("supply/power", tags));

    de.set_actual_number("supply/power", tags, 50);
    QVERIFY(de.value_in_range("supply/power", tags));

    QVERIFY(de.all_values_in_range(tags));
    QVERIFY(de.is_complete(tags));

#endif
}

void Test_Data_engine::test_text_entry() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "test":{
        "data":[
            {	"name": "id",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    Data_engine de{input};
    QMap<QString, QVariant> tags;
    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("test/id", tags));
    de.set_actual_text("test/id", tags, "DEV123456zjuthrgfd");
    QVERIFY(de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("test/id", tags));
    de.set_actual_text("test/id", tags, "DEV123");
    QVERIFY(de.is_complete(tags));
    QVERIFY(de.all_values_in_range(tags));
    QVERIFY(de.value_in_range("test/id", tags));

#endif
}

void Test_Data_engine::test_bool_entry() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "test":{
        "data":[
            {	"name": "checked",	 	"value": true,	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    Data_engine de{input};
    QMap<QString, QVariant> tags;
    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("test/checked", tags));
    de.set_actual_bool("test/checked", tags, false);
    QVERIFY(de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("test/checked", tags));
    de.set_actual_bool("test/checked", tags, true);
    QVERIFY(de.is_complete(tags));
    QVERIFY(de.all_values_in_range(tags));
    QVERIFY(de.value_in_range("test/checked", tags));

#endif
}

void Test_Data_engine::test_empty_entries() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "print":{
        "data":[
            {	"name": "multimeter_sn",	 	"type": "string",	"nice_name": "text just for printing",      "_comment":"test"	},
            {	"name": "24V_variante",         "type": "bool",     "nice_name": "bool just for printing",      "_comment":"test"	},
            {	"name": "number_test",          "type": "number",	"nice_name": "Number just for printing",	"_comment":"test"	}
        ]
    }
}
                            )"};
    Data_engine de{input};
    QMap<QString, QVariant> tags;
    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("print/multimeter_sn", tags));

    de.set_actual_text("print/multimeter_sn", tags, "DEV123");
    QVERIFY(de.value_in_range("print/multimeter_sn", tags));
    de.set_actual_text("print/multimeter_sn", tags, "DEV321");
    QVERIFY(de.value_in_range("print/multimeter_sn", tags));

    QVERIFY(!de.is_complete(tags));

    QVERIFY(!de.value_in_range("print/24V_variante", tags));
    de.set_actual_bool("print/24V_variante", tags, true);
    QVERIFY(de.value_in_range("print/24V_variante", tags));
    de.set_actual_bool("print/24V_variante", tags, false);
    QVERIFY(de.value_in_range("print/24V_variante", tags));

    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));

    QVERIFY(!de.value_in_range("print/number_test", tags));
    de.set_actual_number("print/number_test", tags, 500);
    QVERIFY(de.value_in_range("print/number_test", tags));
    de.set_actual_number("print/number_test", tags, 300);
    QVERIFY(de.value_in_range("print/number_test", tags));



    QVERIFY(de.is_complete(tags));
    QVERIFY(de.all_values_in_range(tags));
#endif
}

void Test_Data_engine::test_references() {
#if !DISABLE_ALL || 1
//TODO: Test if fail when ambiguous
//TODO: Test if fail when desired value is referenced but doesnt exist(actual value only field)
//TODO: Test if fail when unit or si_prefix is defined twice(in source and reference)

//TODO: Test in bool and String environment

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "voltage_source",       "value": "[test_valuesA/voltage_source.actual]",                                        "tolerance": "5",		"nice_name": "Spannungsvergleich"       },
            {	"name": "voltage_color_ist",    "value": "[test_valuesB/voltage_green.actual,        test_valuesB/voltage_blue.actual]","tolerance": 5,         "nice_name": "Farbige Spannung ist"     },
            {	"name": "voltage_color_soll",   "value": "[test_valuesB/voltage_green.desired,     test_valuesB/voltage_blue.desired]", "tolerance": "5",		"nice_name": "Farbige Spannung soll"    }
        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "voltage_source",       "type": "number",                   "unit": "mV",   "si_prefix": 1e-3,  "nice_name": "text just for printing"	}
        ]
    },
    "test_valuesB":[
        {
            "apply_if":{
                "color":"blue"
            },
            "data":[
               {	"name": "voltage_blue",     "value": 100,   "tolerance": 25,	"unit": "mV",   "si_prefix": 1e-3,  "nice_name": "Spannung blau"            }
            ]
        },
        {
            "apply_if":{
                "color":"green"
            },
            "data":[
                {	"name": "voltage_green",    "value": 50,    "tolerance": 10,	"unit": "mV",   "si_prefix": 1e-3,  "nice_name": "Spannung gruen"           }
            ]
        }
    ]

}
                            )"};
    Data_engine de{input};
    QMap<QString, QVariant> tags;
    tags.insert("color","green");
    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));
    QVERIFY(!de.value_in_range("referenzen/voltage_source", tags));

    de.set_actual_number("test_valuesA/voltage_source", tags, 500);
    QVERIFY(de.value_in_range("test_valuesA/voltage_source", tags));
    QVERIFY(!de.value_in_range("referenzen/voltage_source", tags));
    de.set_actual_number("referenzen/voltage_source", tags, 506);
    QVERIFY(!de.value_in_range("referenzen/voltage_source", tags));
    de.set_actual_number("referenzen/voltage_source", tags, 505);

    QVERIFY(de.value_in_range("referenzen/voltage_source", tags));

    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));


    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist", tags));
    de.set_actual_number("referenzen/voltage_color_ist", tags, 300);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist", tags));

    de.set_actual_number("test_valuesB/voltage_green", tags, 306);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist", tags));

    de.set_actual_number("test_valuesB/voltage_green", tags, 305);
    QVERIFY(de.value_in_range("referenzen/voltage_color_ist", tags));

    QVERIFY(!de.is_complete(tags));
    QVERIFY(!de.all_values_in_range(tags));


    QVERIFY(!de.value_in_range("referenzen/voltage_color_soll", tags));
    de.set_actual_number("referenzen/voltage_color_soll", tags, 56);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_soll", tags));

    de.set_actual_number("test_valuesB/voltage_green", tags, 55);
    QVERIFY(de.value_in_range("referenzen/voltage_color_soll", tags));


    QVERIFY(de.is_complete(tags));
    QVERIFY(de.all_values_in_range(tags));
#endif
}
void Test_Data_engine::test_preview() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{
    "supply":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Spannung +5V"	},
            {	"name": "current",	 	"value": 100,	"tolerance": "5%",	"unit": "mA", "si_prefix": 1e-3,	"nice_name": "Strom +5V"	}
        ]
    }
}
                            )"};

    QMap<QString, QVariant> tags;

	int argc = 1;
	char executable[] = "";
	char *executable2 = executable;
	char **argv = &executable2;
	QApplication app(argc, argv);

	Data_engine de{input};
    de.set_actual_number("supply/voltage",tags, 5000);
    de.set_actual_number("supply/current", tags, 150.);
	de.generate_pdf("test.xml", "test.pdf");

#endif
}
