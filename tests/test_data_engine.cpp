#include "test_data_engine.h"
#include "data_engine/data_engine.h"

#include <QList>
#include <QPair>
#include <QString>
#include <sstream>

#define DISABLE_ALL 0

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
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};
#endif
}

void Test_Data_engine::check_properties_of_empty_set() {
#if !DISABLE_ALL
    std::stringstream input{R"({})"};
    QMap<QString, QVariant> tags;
    const Data_engine de{input, tags};
    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
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
    QMap<QString, QVariant> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::no_data_section_found);
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
    QMap<QString, QVariant> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::no_data_section_found);
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
    QMap<QString, QVariant> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::tolerance_parsing_error);
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
    QMap<QString, QVariant> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::duplicate_field);
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

    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage"), DataEngineErrorNumber::faulty_field_id);

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

    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage/voltage"), DataEngineErrorNumber::no_field_id_found);

#endif
}

struct TestDependencyData {
    QList<QPair<QString, QVariant>> values;

    DataEngineErrorNumber expected_error;
};

void Test_Data_engine::check_dependency_handling() {
#if !DISABLE_ALL || 0
    std::vector<TestDependencyData> in_data{

        {{{"sound", 1.2}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::no_variant_found}, //
        {{{"sound", 1.42}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.5}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.8}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", false}},
         DataEngineErrorNumber::no_variant_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "6V"}, {"high_power_edidtion", false}},
         DataEngineErrorNumber::no_variant_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"color", "green"}, {"supply_version", "12V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::no_variant_found},
        {{{"sound", 1.42}, {"main", 1.42}, {"power_supply", 1.9}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::no_variant_found},
        {{{"sound", 1.442}, {"main", 1.42}, {"power_supply", 1.5}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::no_variant_found},
        {{{"sound", 1.442}, {"main", 1.42}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::no_variant_found}

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

        QMap<QString, QVariant> tags;
        QString printer;
        for (const auto &value : data.values) {
            tags.insert(value.first, value.second);
            printer += value.first + ": " + value.second.toString() + ", ";
        }

        // qDebug() << printer;
        if (data.expected_error == DataEngineErrorNumber::no_variant_found) {
            Data_engine de(input, tags);
            QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage/voltage"), data.expected_error);

        } else {
            Data_engine de(input, tags);
            de.get_desired_value_as_string("supply-voltage/voltage");
        }
    }
#endif
}

void Test_Data_engine::check_dependency_ambiguity_handling() {
#if !DISABLE_ALL || 0
    std::vector<TestDependencyData> in_data{

        {{{"sound", 1.2}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::no_variant_found}, //
        {{{"sound", 1.42}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok}, //
        {{{"sound", 1.43}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::non_unique_desired_field_found}, //
        {{{"sound", 1.442}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok}, //
        {{{"sound", 1.5}, {"main", 1.43}, {"power_supply", 1.55}, {"color", "green"}, {"supply_version", "24V"}, {"high_power_edidtion", true}},
         DataEngineErrorNumber::ok}, //
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

        QMap<QString, QVariant> tags;
        QString printer;
        for (const auto &value : data.values) {
            tags.insert(value.first, value.second);
            printer += value.first + ": " + value.second.toString() + ", ";
        }
        //qDebug() << printer;

        if (data.expected_error == DataEngineErrorNumber::non_unique_desired_field_found) {
            QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags), data.expected_error);
        } else if (data.expected_error == DataEngineErrorNumber::no_variant_found) {
            Data_engine de(input, tags);
            QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage/voltage"), data.expected_error);

        } else {
            Data_engine de(input, tags);
            de.get_desired_value_as_string("supply-voltage/voltage");
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

    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage/voltage"), DataEngineErrorNumber::no_section_id_found);

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
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};
    de.get_desired_value_as_string("supply-voltage/voltage");

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
    QMap<QString, QVariant> tags;
    const Data_engine de{input, tags};
    QCOMPARE(de.get_desired_value_as_string("supply-voltage/voltage"), QString("5000 (±200)"));
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

    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("supply/voltage"));

    QCOMPARE(de.get_desired_value_as_string("supply/voltage"), QString("5000 (±200)"));
    QCOMPARE(de.get_unit("supply/voltage"), "mV"_qs);

    de.set_actual_number("supply/voltage", 5201);
    QVERIFY(!de.value_in_range("supply/voltage"));

    de.set_actual_number("supply/voltage", 5200);
    QVERIFY(de.value_in_range("supply/voltage"));

    de.set_actual_number("supply/current", 106);
    QVERIFY(!de.value_in_range("supply/current"));

    de.set_actual_number("supply/current", 105);
    QVERIFY(de.value_in_range("supply/current"));

    de.set_actual_number("supply/power", 105);
    QVERIFY(!de.value_in_range("supply/power"));

    de.set_actual_number("supply/power", 104);
    QVERIFY(de.value_in_range("supply/power"));

    de.set_actual_number("supply/power", 50);
    QVERIFY(de.value_in_range("supply/power"));

    QVERIFY(de.all_values_in_range());
    QVERIFY(de.is_complete());

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
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("test/id"));
    de.set_actual_text("test/id", "DEV123456zjuthrgfd");
    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("test/id"));
    de.set_actual_text("test/id", "DEV123");
    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
    QVERIFY(de.value_in_range("test/id"));

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
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("test/checked"));
    de.set_actual_bool("test/checked", false);
    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("test/checked"));
    de.set_actual_bool("test/checked", true);
    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
    QVERIFY(de.value_in_range("test/checked"));

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
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("print/multimeter_sn"));

    de.set_actual_text("print/multimeter_sn", "DEV123");
    QVERIFY(de.value_in_range("print/multimeter_sn"));
    de.set_actual_text("print/multimeter_sn", "DEV321");
    QVERIFY(de.value_in_range("print/multimeter_sn"));

    QVERIFY(!de.is_complete());

    QVERIFY(!de.value_in_range("print/24V_variante"));
    de.set_actual_bool("print/24V_variante", true);
    QVERIFY(de.value_in_range("print/24V_variante"));
    de.set_actual_bool("print/24V_variante", false);
    QVERIFY(de.value_in_range("print/24V_variante"));

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.value_in_range("print/number_test"));
    de.set_actual_number("print/number_test", 500);
    QVERIFY(de.value_in_range("print/number_test"));
    de.set_actual_number("print/number_test", 300);
    QVERIFY(de.value_in_range("print/number_test"));

    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
#endif
}

void Test_Data_engine::test_if_fails_when_desired_number_misses_tolerance() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "test_valuesA":{
        "data":[
            {	"name": "test_string",                 "value": 500,              "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::tolerance_must_be_defined_for_numbers);
#endif
}

void Test_Data_engine::test_if_success_when_actuel_number_misses_tolerance() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "test_valuesA":{
        "data":[
            {	"name": "test_string",                 "type": "number",              "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;
    Data_engine(input, tags);
#endif
}

void Test_Data_engine::test_references() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "voltage_source",       "value": "[test_valuesA/voltage_source.actual,  test_valuesB/voltage_blue.actual]",                                        "tolerance": "5",		"nice_name": "Spannungsvergleich"       },
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
    QMap<QString, QVariant> tags;
    tags.insert("color", "green");
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("referenzen/voltage_source"));

    de.set_actual_number("test_valuesA/voltage_source", 500);
    QVERIFY(de.value_in_range("test_valuesA/voltage_source"));
    QVERIFY(!de.value_in_range("referenzen/voltage_source"));
    de.set_actual_number("referenzen/voltage_source", 506);
    QVERIFY(!de.value_in_range("referenzen/voltage_source"));
    de.set_actual_number("referenzen/voltage_source", 505);

    QVERIFY(de.value_in_range("referenzen/voltage_source"));

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist"));
    de.set_actual_number("referenzen/voltage_color_ist", 300);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist"));

    de.set_actual_number("test_valuesB/voltage_green", 306);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist"));

    de.set_actual_number("test_valuesB/voltage_green", 305);
    QVERIFY(de.value_in_range("referenzen/voltage_color_ist"));

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.value_in_range("referenzen/voltage_color_soll"));
    de.set_actual_number("referenzen/voltage_color_soll", 56);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_soll"));

    de.set_actual_number("referenzen/voltage_color_soll", 55);
    QVERIFY(de.value_in_range("referenzen/voltage_color_soll"));

    de.set_actual_number("test_valuesB/voltage_green", 60);
    de.set_actual_number("referenzen/voltage_color_ist", 65);

    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
#endif
}

void Test_Data_engine::test_references_ambiguous() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "voltage_source",       "value": "[test_valuesA/voltage_source.actual,  test_valuesB/voltage_green.actual]",                                        "tolerance": "5",		"nice_name": "Spannungsvergleich"       }

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
    QMap<QString, QVariant> tags;
    tags.insert("color", "green");

    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::reference_ambiguous);
#endif
}

void Test_Data_engine::test_references_non_existing() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "voltage_source",       "value": "[test_valuesA/voltage_sourceas.actual,  test_valuesB/voltage_greendas.actual]",                                        "tolerance": "5",		"nice_name": "Spannungsvergleich"       }

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
    QMap<QString, QVariant> tags;
    tags.insert("color", "green");
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::reference_not_found);

#endif
}

void Test_Data_engine::test_references_from_non_actual_only_field() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "voltage_source",       "value": "[test_valuesA/voltage_source.desired]",                                        "tolerance": "5",		"nice_name": "Spannungsvergleich"       }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "voltage_source",       "type": "number",                   "unit": "mV",   "si_prefix": 1e-3,  "nice_name": "text just for printing"	}
        ]
    }


}
                            )"};
    QMap<QString, QVariant> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(

        Data_engine(input, tags), DataEngineErrorNumber::reference_target_has_no_desired_value);

#endif
}

void Test_Data_engine::test_references_get_actual_value_description_desired_value() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[

            {	"name": "test_number_ref",          "value": "[test_valuesA/test_number.desired]",          "tolerance":2 ,                "nice_name": "Referenz zum numer soll"         },
            {	"name": "test_bool_ref",            "value": "[test_valuesA/test_bool.desired]",                        "nice_name": "Referenz zum bool soll"           },
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_string.desired]",                      "nice_name": "Referenz zum string soll"        }
        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_number",                  "value": 100,     "tolerance":1 ,   "unit":"mA",           "nice_name": "Original number"        },
            {	"name": "test_bool",                    "value": true,                                  "nice_name": "Original bool"	},
            {	"name": "test_string",                  "value": "test_string",                         "nice_name": "Original string"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    de.set_actual_number("test_valuesA/test_number", 101);
    de.set_actual_bool("test_valuesA/test_bool", true);
    de.set_actual_text("test_valuesA/test_string", "TEST321");

    de.set_actual_number("referenzen/test_number_ref", 102);
    de.set_actual_bool("referenzen/test_bool_ref", true);
    de.set_actual_text("referenzen/test_string_ref", "TEST123");

    QCOMPARE(de.get_actual_value("test_valuesA/test_number"),QString("101"));
    QCOMPARE(de.get_description("test_valuesA/test_number"),QString("Original number"));
    QCOMPARE(de.get_desired_value_as_string("test_valuesA/test_number"),QString("100 (±1)"));
    QCOMPARE(de.get_unit("test_valuesA/test_number"),QString("mA"));

    QCOMPARE(de.get_actual_value("test_valuesA/test_bool"),QString("true"));
    QCOMPARE(de.get_description("test_valuesA/test_bool"),QString("Original bool"));
    QCOMPARE(de.get_desired_value_as_string("test_valuesA/test_bool"),QString("true"));
    QCOMPARE(de.get_unit("test_valuesA/test_bool"),QString(""));

    QCOMPARE(de.get_actual_value("test_valuesA/test_string"),QString("TEST321"));
    QCOMPARE(de.get_description("test_valuesA/test_string"),QString("Original string"));
    QCOMPARE(de.get_desired_value_as_string("test_valuesA/test_string"),QString("test_string"));
    QCOMPARE(de.get_unit("test_valuesA/test_string"),QString(""));

    QCOMPARE(de.get_actual_value("referenzen/test_number_ref"),QString("102"));
    QCOMPARE(de.get_description("referenzen/test_number_ref"),QString("Referenz zum numer soll"));
    QCOMPARE(de.get_desired_value_as_string("referenzen/test_number_ref"),QString("100 (±2)"));
    QCOMPARE(de.get_unit("referenzen/test_number_ref"),QString("mA"));

    QCOMPARE(de.get_actual_value("referenzen/test_bool_ref"),QString("true"));
    QCOMPARE(de.get_description("referenzen/test_bool_ref"),QString("Referenz zum bool soll"));
    QCOMPARE(de.get_desired_value_as_string("referenzen/test_bool_ref"),QString("true"));
    QCOMPARE(de.get_unit("referenzen/test_bool_ref"),QString(""));

    QCOMPARE(de.get_actual_value("referenzen/test_string_ref"),QString("TEST123"));
    QCOMPARE(de.get_description("referenzen/test_string_ref"),QString("Referenz zum string soll"));
    QCOMPARE(de.get_desired_value_as_string("referenzen/test_string_ref"),QString("test_string"));
    QCOMPARE(de.get_unit("referenzen/test_string_ref"),QString(""));


#endif
}

void Test_Data_engine::test_references_string_bool() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_bool_ao_ist_ref",         "value": "[test_valuesA/test_bool_actual_only.actual]",           	"nice_name": "Referenz zum bool AO ist"         },
            {	"name": "test_string_ao_ist_ref",       "value": "[test_valuesA/test_string_actual_only.actual]",           "nice_name": "Referenz zum string AO ist"       },

            {	"name": "test_bool_ist_ref",            "value": "[test_valuesA/test_bool.actual]",                         "nice_name": "Referenz zum bool ist"            },
            {	"name": "test_string_ist_ref",          "value": "[test_valuesA/test_string.actual]",                       "nice_name": "Referenz zum string ist "         },
            {	"name": "test_bool_soll_ref",           "value": "[test_valuesA/test_bool.desired]",                        "nice_name": "Referenz zum bool soll"           },
            {	"name": "test_string_soll_ref",         "value": "[test_valuesA/test_string.desired]",                      "nice_name": "Referenz zum string soll "        }
        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_bool_actual_only",       "type": "bool",                      "nice_name": "Actual Value only field in bool"          },
            {	"name": "test_string_actual_only",     "type": "string",                    "nice_name": "Actual Value only field in string"        },
            {	"name": "test_bool",                   "value": true,                       "nice_name": "Actual and Desired Value field in bool"	},
            {	"name": "test_string",                 "value": "test_string",              "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.value_in_range("referenzen/test_bool_ao_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ao_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_bool_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ist_ref"));

    QVERIFY(!de.value_in_range("referenzen/test_string_soll_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_bool_soll_ref"));

    de.set_actual_bool("test_valuesA/test_bool_actual_only", true);
    de.set_actual_text("test_valuesA/test_string_actual_only", "TEST123");

    QVERIFY(!de.value_in_range("referenzen/test_bool_ao_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ao_ist_ref"));

    de.set_actual_bool("referenzen/test_bool_ao_ist_ref", false);
    de.set_actual_text("referenzen/test_string_ao_ist_ref", "TEST123_fail");

    QVERIFY(!de.value_in_range("referenzen/test_bool_ao_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ao_ist_ref"));

    de.set_actual_bool("referenzen/test_bool_ao_ist_ref", true);
    de.set_actual_text("referenzen/test_string_ao_ist_ref", "TEST123");

    QVERIFY(de.value_in_range("referenzen/test_bool_ao_ist_ref"));
    QVERIFY(de.value_in_range("referenzen/test_string_ao_ist_ref"));

    de.set_actual_bool("test_valuesA/test_bool", false);
    de.set_actual_text("test_valuesA/test_string", "TEST123");

    QVERIFY(!de.value_in_range("referenzen/test_bool_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ist_ref"));

    QVERIFY(!de.value_in_range("referenzen/test_bool_soll_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_soll_ref"));

    de.set_actual_bool("referenzen/test_bool_ist_ref", true);
    de.set_actual_text("referenzen/test_string_ist_ref", "TEST123_fail");
    de.set_actual_bool("referenzen/test_bool_soll_ref", false);
    de.set_actual_text("referenzen/test_string_soll_ref", "TEST123_fail");

    QVERIFY(!de.value_in_range("referenzen/test_bool_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_bool_soll_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_soll_ref"));

    de.set_actual_bool("referenzen/test_bool_ist_ref", false);
    de.set_actual_text("referenzen/test_string_ist_ref", "TEST123");
    de.set_actual_bool("referenzen/test_bool_soll_ref", true);
    de.set_actual_text("referenzen/test_string_soll_ref", "test_string");

    QVERIFY(de.value_in_range("referenzen/test_bool_ist_ref"));
    QVERIFY(de.value_in_range("referenzen/test_string_ist_ref"));
    QVERIFY(de.value_in_range("referenzen/test_bool_soll_ref"));
    QVERIFY(de.value_in_range("referenzen/test_string_soll_ref"));

    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_actual_bool("test_valuesA/test_bool", true);
    de.set_actual_text("test_valuesA/test_string", "test_string");

    QVERIFY(!de.value_in_range("referenzen/test_bool_ist_ref"));
    QVERIFY(!de.value_in_range("referenzen/test_string_ist_ref"));
    QVERIFY(de.value_in_range("test_valuesA/test_bool"));
    QVERIFY(de.value_in_range("test_valuesA/test_string"));

    de.set_actual_bool("referenzen/test_bool_ist_ref", true);
    de.set_actual_text("referenzen/test_string_ist_ref", "test_string");

    QVERIFY(de.all_values_in_range());
#endif
}


void Test_Data_engine::test_references_set_value_in_wrong_type() {
#if !DISABLE_ALL

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_number_ref",          "value": "[test_valuesA/test_number.desired]",    "tolerance": 20,        	"nice_name": "Referenz zum bool AO ist"         },
            {	"name": "test_bool_ref",            "value": "[test_valuesA/test_bool.desired]",                 "nice_name": "Referenz zum string AO ist"       },
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_string.desired]",               "nice_name": "Referenz zum bool ist"            }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_number",                 "value": 500,          "tolerance": 200,              "nice_name": "Actual Value only field in string"        },
            {	"name": "test_bool",                   "value": true,                       "nice_name": "Actual and Desired Value field in bool"	},
            {	"name": "test_string",                 "value": "test_string",              "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_bool("referenzen/test_number_ref", true),
                                          DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_text("referenzen/test_bool_ref", "TEST123_fail"),
                                          DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("referenzen/test_string_ref", 500),
                                          DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type);

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_text("referenzen/test_number_ref", "TEST_FAIl"),
                                          DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("referenzen/test_bool_ref", 500),
                                          DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_bool("referenzen/test_string_ref", true),
                                          DataEngineErrorNumber::setting_reference_actual_value_with_wrong_type);

#endif
}

void Test_Data_engine::test_references_if_fails_when_setting_tolerance_in_bool() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_bool_ref",            "value": "[test_valuesA/test_bool.desired]",             "tolerance": 25,	   "nice_name": "Referenz zum string AO ist"       }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_bool",                   "value": true,                       "nice_name": "Actual and Desired Value field in bool"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::reference_is_not_number_but_has_tolerance);
#endif
}

void Test_Data_engine::test_references_if_fails_when_setting_tolerance_in_string() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_string.desired]",           "tolerance": 25,	    "nice_name": "Referenz zum bool ist"            }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_string",                 "value": "test_string",              "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::reference_is_not_number_but_has_tolerance);
#endif
}


void Test_Data_engine::test_references_when_number_reference_without_tolerance() {
#if !DISABLE_ALL || 0

    std::stringstream input_ok{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_bool_ref",            "value": "[test_valuesA/test_bool.desired]",             "tolerance": 30,	   "nice_name": "Referenz ist ok"       }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_bool",                   "value": 123,      "tolerance": 25,                  "nice_name": "nummer"	}
        ]
    }

}
                            )"};

    std::stringstream input_fail{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_bool_ref",            "value": "[test_valuesA/test_bool.desired]",           	   "nice_name": "Referenz ist ok"       }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_bool",                   "value": 123,      "tolerance": 25,                  "nice_name": "nummer"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;
    Data_engine(input_ok, tags);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_fail, tags);, DataEngineErrorNumber::reference_is_a_number_and_needs_tolerance);
#endif
}

void Test_Data_engine::test_references_if_fails_when_mismatch_in_unit() {
#if !DISABLE_ALL || 0
    std::stringstream input_ok{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_number_unit_prefix.desired]",           "tolerance": 25,	    "nice_name": "Referenz zum bool ist"            }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_number_unit_prefix",                 "value": 123,       "tolerance": 25, "unit":"mA",   "si_prefix": 1e-3, "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};

    std::stringstream input_fail_unit{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_number_unit_prefix.desired]",      "unit":"mA",     "tolerance": 25,	    "nice_name": "Referenz zum bool ist"            }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_number_unit_prefix",                 "value": 123,       "tolerance": 25, "unit":"mA",   "si_prefix": 1e-3, "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};

    std::stringstream input_fail_prefix{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_number_unit_prefix.desired]",      "si_prefix": 1e-3,     "tolerance": 25,	    "nice_name": "Referenz zum bool ist"            }

        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_number_unit_prefix",                 "value": 123,       "tolerance": 25, "unit":"mA",   "si_prefix": 1e-3, "nice_name": "Actual and Desired Value field in strign"	}
        ]
    }

}
                            )"};
    QMap<QString, QVariant> tags;

    Data_engine(input_ok, tags);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_fail_unit, tags);,      DataEngineErrorNumber::invalid_data_entry_key);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_fail_prefix, tags);,    DataEngineErrorNumber::invalid_data_entry_key);
#endif
}


void Test_Data_engine::test_preview() {
#if !DISABLE_ALL && 0
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

    Data_engine de{input, tags};
    de.set_actual_number("supply/voltage", 5000);
    de.set_actual_number("supply/current", 150.);
    de.generate_pdf("test.xml", "test.pdf");

#endif
}

