#include "test_data_engine.h"
#include "LuaFunctions/lua_functions.h"
#include "data_engine/data_engine.h"
#include "data_engine/exceptionalapproval.h"

#include <experimental/optional>

#include "config.h"

#include <QList>
#include <QPair>
#include <QSettings>
#include <QString>
#include <QTemporaryFile>
#include <sstream>

#define DISABLE_ALL 0
//
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
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
#endif
}

void Test_Data_engine::check_properties_of_empty_set() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"({})"};
    QMap<QString, QList<QVariant>> tags;
    const Data_engine de{input, tags};
    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
#endif
}

void Test_Data_engine::stl_optional_test() {
    std::experimental::optional<int> desired_value{};

    QVERIFY(desired_value.value_or(100) == 100);
    desired_value = 10;
    QVERIFY(desired_value.value_or(100) == 10);
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
    QMap<QString, QList<QVariant>> tags;
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
    QMap<QString, QList<QVariant>> tags;
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
    QMap<QString, QList<QVariant>> tags;
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
    QMap<QString, QList<QVariant>> tags;
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

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
    QCOMPARE(de.get_desired_number("supply-voltage/voltage_test"), 5000.0);
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

    QMap<QString, QList<QVariant>> tags;
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

        QMap<QString, QList<QVariant>> tags;
        QString printer;
        for (const auto &value : data.values) {
            tags.insert(value.first, {value.second});
            printer += value.first + ": " + value.second.toString() + ", ";
        }

        //qDebug() << printer;
        if (data.expected_error == DataEngineErrorNumber::no_variant_found) {
            QVERIFY_EXCEPTION_THROWN_error_number(Data_engine de(input, tags);, data.expected_error);

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

        QMap<QString, QList<QVariant>> tags;
        QString printer;
        for (const auto &value : data.values) {
            tags.insert(value.first, {value.second});
            printer += value.first + ": " + value.second.toString() + ", ";
        }
        // qDebug() << printer;

        if (data.expected_error == DataEngineErrorNumber::non_unique_desired_field_found) {
            QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags), data.expected_error);
        } else if (data.expected_error == DataEngineErrorNumber::no_variant_found) {
            QVERIFY_EXCEPTION_THROWN_error_number(Data_engine de(input, tags);, data.expected_error);

        } else {
            Data_engine de(input, tags);
            de.get_desired_value_as_string("supply-voltage/voltage");
        }
    }
#endif
}

void Test_Data_engine::check_emtpy_section_tag() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
    {
        "supply-voltage":{
            "allow_empty_section": true,
            "apply_if":{
                "sound":true
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    }
                                )"};

    QMap<QString, QList<QVariant>> tags;
    Data_engine de(input, tags);
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage/voltage");, DataEngineErrorNumber::no_field_id_found);
#endif
}

void Test_Data_engine::check_emtpy_section_tag_wrong_type() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
    {
        "supply-voltage":{
            "allow_empty_section": 56,
            "apply_if":{
                "sound":true
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    }
                                )"};

    QMap<QString, QList<QVariant>> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine de(input, tags);, DataEngineErrorNumber::allow_empty_section_with_wrong_type);
#endif
}

void Test_Data_engine::check_emtpy_section_tag_wrong_scope() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "supply-voltage":[
        {
            "apply_if":{
                "sound":"[1.42-1.442]"
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        },
        {
            "allow_empty_section":true,
            "apply_if":{
                "sound":"[1.43-*]"
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    ]
}
                            )"};

    QMap<QString, QList<QVariant>> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::allow_empty_section_must_not_be_defined_in_variant_scope);

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

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_value_as_string("supply-voltage/voltage"), DataEngineErrorNumber::no_section_id_found);

#endif
}

void Test_Data_engine::check_data_by_object() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{
    "supply-voltage":{
        "title":"Supply-Voltage",
        "data":
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
    de.get_desired_value_as_string("supply-voltage/voltage");
    QCOMPARE(de.get_section_title("supply-voltage"), QString("Supply-Voltage"));
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
    QMap<QString, QList<QVariant>> tags;
    const Data_engine de{input, tags};
    QCOMPARE(de.get_desired_value_as_string("supply-voltage/voltage"), QString{"5000 (±200) mV"});
#endif
}

void Test_Data_engine::test_dummy_data_generation() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "supply":{
        "data":[
            {	"name": "test_number",	 	"type": "number",		"unit": "mV", 	"nice_name": "Nummer"	},
            {	"name": "test_string",	 	"type": "string",                      	"nice_name": "Text"	},
            {	"name": "test_bool",	 	"type": "bool",                      	"nice_name": "bool"	},
            {	"name": "test_datetime",	 "type": "datetime",                   	"nice_name": "Date time"	}

        ]
    }
}
                            )"};

    Data_engine de{input};
    de.fill_engine_with_dummy_data();

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_value("supply/test_number"), DataEngineErrorNumber::is_in_dummy_mode);
    QCOMPARE(de.get_actual_dummy_value("supply/test_number"), QString("1 mV"));
    QCOMPARE(de.get_actual_dummy_value("supply/test_string"), QString("test 123"));
    QCOMPARE(de.get_actual_dummy_value("supply/test_bool"), QString("Yes"));
    QString date_str = de.get_actual_dummy_value("supply/test_datetime").split(".")[0]; //remove milliseconds
                                                                                        //  qDebug() << date_str;
    QDateTime prope_date_time = DataEngineDateTime(date_str).dt();
    QVERIFY(prope_date_time.isValid());

#endif
}

void Test_Data_engine::single_numeric_property_test() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
	"supply":{
		"data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "voltage_neg",	"value": -100.5,"tolerance": "10%",	"unit": "mV", "si_prefix": 1,	"nice_name": "Betriebsspannung +5V"	},
			{	"name": "current",	 	"value": 100,	"tolerance": "5%",	"unit": "mA", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	},
			{	"name": "power",	 	"value": 100,	"tolerance": "+4/*",	"unit": "mW", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
		]
	}
}
							)"};

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("supply/voltage"));
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_dummy_value("supply/voltage"), DataEngineErrorNumber::dummy_mode_necessary);
    QCOMPARE(de.get_desired_value_as_string("supply/voltage"), QString("5000 (±200) mV"));
    QCOMPARE(de.get_unit("supply/voltage"), QString{"mV"});

    de.set_actual_number("supply/voltage", NAN);
    QVERIFY(!de.value_in_range("supply/voltage"));

    de.set_actual_number("supply/voltage", 5.201);
    QVERIFY(!de.value_in_range("supply/voltage"));

    de.set_actual_number("supply/voltage", 5.200);
    QVERIFY(de.value_in_range("supply/voltage"));

    de.set_actual_number("supply/voltage_neg", -90.44);
    QVERIFY(!de.value_in_range("supply/voltage_neg"));

    de.set_actual_number("supply/voltage_neg", -90.46);
    QVERIFY(de.value_in_range("supply/voltage_neg"));

    de.set_actual_number("supply/voltage_neg", -120);
    QVERIFY(!de.value_in_range("supply/voltage_neg"));

    de.set_actual_number("supply/voltage_neg", -91);
    QVERIFY(de.value_in_range("supply/voltage_neg"));

    de.set_actual_number("supply/voltage_neg", -109);
    QVERIFY(de.value_in_range("supply/voltage_neg"));

    de.set_actual_number("supply/current", 0.106);
    QVERIFY(!de.value_in_range("supply/current"));

    de.set_actual_number("supply/current", 0.105);
    QVERIFY(de.value_in_range("supply/current"));

    de.set_actual_number("supply/power", 0.105);
    QVERIFY(!de.value_in_range("supply/power"));

    de.set_actual_number("supply/power", 0.104);
    QVERIFY(de.value_in_range("supply/power"));

    de.set_actual_number("supply/power", 0.05);
    QVERIFY(de.value_in_range("supply/power"));

    QVERIFY(de.all_values_in_range());
    QVERIFY(de.is_complete());

#endif
}

void Test_Data_engine::test_text_entry() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "test":{
        "data":[
            {	"name": "id",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
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

void Test_Data_engine::test_iterate_entries() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "testA":{
        "data":[
            {	"name": "idA1",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "idA2",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
        ]
    },
    "testB":{
        "data":[
            {	"name": "idB1",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "idB2",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "idB3",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "idB4",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QStringList slA = de.get_ids_of_section("testA");
    QCOMPARE(slA.count(), 2);
    QCOMPARE(slA[0], QString("testA/idA1"));
    QCOMPARE(slA[1], QString("testA/idA2"));

    auto section = de.get_section_names();
    QCOMPARE(section.count(), 2);
    QCOMPARE(section[0], QString("testA"));
    QCOMPARE(section[1], QString("testB"));

    auto instances = de.get_instance_captions("testA");
    QCOMPARE(instances.count(), 1);
    QCOMPARE(instances[0], QString(""));

#endif
}

void Test_Data_engine::test_section_valid() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "testA":{
        "data":[
            {	"name": "idA1",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "idA2",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
        ]
    },
    "testB":{
            "data":[
                        {	"name": "idB1_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                        {	"name": "idB2_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                        {	"name": "idB3_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                        {	"name": "idB4_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
            ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QVERIFY(!de.value_complete_in_section("testA"));
    QVERIFY(!de.value_in_range_in_section("testA"));
    QVERIFY(!de.values_in_range(QList<FormID>{"testA/idA1", "testA/idA2"}));

    de.set_actual_text("testA/idA1", "DEV123");
    de.set_actual_text("testA/idA2", "DEV122");
    QVERIFY(!de.values_in_range(QList<FormID>{"testA/idA1", "testA/idA2"}));
    QVERIFY(de.value_complete_in_section("testA"));
    QVERIFY(!de.value_in_range_in_section("testA"));
    de.set_actual_text("testA/idA2", "DEV123");
    QVERIFY(de.values_in_range(QList<FormID>{"testA/idA1", "testA/idA2"}));
    QVERIFY(de.value_in_range_in_section("testA"));

    QVERIFY(!de.value_in_range_in_section("testB"));
#endif
}

void Test_Data_engine::test_iterate_entries_instance() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "testA":{
        "data":[
            {	"name": "idA1",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
            {	"name": "idA2",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
        ]
    },
    "testB":{
       "instance_count": "2",
        "variants":[
            {
                "apply_if": {
                    "foo": true
                },
                "data":[
                            {	"name": "idB1_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                            {	"name": "idB2_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                            {	"name": "idB3_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                            {	"name": "idB4_true",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
                ]

            },
            {
                "apply_if": {
                    "foo": false
                },
                "data":[
                            {	"name": "idB1_false",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                            {	"name": "idB2_false",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                            {	"name": "idB3_false",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	},
                            {	"name": "idB4_false",	 	"value": "DEV123",	"nice_name": "Betriebsspannung +5V"	}
                ]

            }

        ]

    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags["foo"] = {true, false};
    Data_engine de{input, tags};

    QStringList ref{"testA/idA1",      "testA/idA2",       "testB/idB1_true",  "testB/idB2_true",  "testB/idB3_true",
                    "testB/idB4_true", "testB/idB1_false", "testB/idB2_false", "testB/idB3_false", "testB/idB4_false"};
    uint ref_index = 0;
    auto sections = de.get_section_names();

    for (auto section : sections) {
        auto instances = de.get_instance_captions(section);
        uint i = 1;
        for (auto instance : instances) {
            de.use_instance(section, "", i);
            QStringList ids = de.get_ids_of_section(section);
            for (auto id : ids) {
                //qDebug() << id;
                QCOMPARE(id, ref[ref_index]);
                ref_index++;
            }
            i++;
        }
    }

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
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_number("test/checked"), DataEngineErrorNumber::desired_value_is_not_a_number);
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

void Test_Data_engine::test_dateime_entry() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "test":{
        "data":[
            {	"name": "date_today",	 	"type": "datetime",	 "nice_name": "Heute"	}
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("test/date_today"));
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_number("test/date_today"), DataEngineErrorNumber::desired_value_is_not_a_number);

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_datetime("test/date_today", QDateTime());, DataEngineErrorNumber::datetime_is_not_valid);

    QDateTime ref_date = QDateTime::fromString("2018:01:01 13:45:50", "yyyy:MM:dd HH:mm:ss");
    de.set_actual_datetime("test/date_today", ref_date);
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_number("test/date_today");, DataEngineErrorNumber::actual_value_is_not_a_number);
    QCOMPARE(de.get_actual_value("test/date_today"), QString("2018-01-01 13:45:50"));
    QVERIFY(de.is_complete());
    QVERIFY(de.all_values_in_range());
    QVERIFY(de.value_in_range("test/date_today"));
#endif
}

void Test_Data_engine::test_dateime_entry_2() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "test":{
        "data":[
            {	"name": "date_today",	 	"type": "datetime",	 "value": "2018-01-01 13:45:50",  "nice_name": "Heute"	}
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::invalid_data_entry_key);

#endif
}

void Test_Data_engine::test_dateime_entry_3() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "test":{
        "data":[
            {	"name": "date_today",	 		 "value": "2018-01-01 13:45:50",  "nice_name": "Heute"	}
        ]
    }
}
                            )"};

    // the field "test/date_today" is a text field and cannot be set by datetime.

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("test/date_today"));
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_datetime("test/date_today", QDateTime::fromString("2018:01:01 13:45:50", "yyyy:MM:dd HH:mm:ss"));
                                          , DataEngineErrorNumber::setting_desired_value_with_wrong_type);
#endif
}

void Test_Data_engine::test_instances() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "supply_1":{
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "One Instance. Normal mode"	}
        ]
    },
    "supply_2":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Two Instances. Fixed mode"	}
        ]
    },
    "supply_probe_count":{
        "instance_count": "probe_count",
        "variants":[
            {
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. One Variant"	}
                ]
            }
        ]
    },
    "supply_2_object":{
        "instance_count": "2",
        "variants":{
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. One Variant"	}
                ]
            }
    },
    "supply_varants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is not chosen"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_actual_number("supply_1/voltage", 5.0);

    de.set_actual_number("supply_2/voltage", 7.0);
    QVERIFY(!de.value_in_range_in_instance("supply_2/voltage"));
    de.set_actual_number("supply_2/voltage", 5.0);
    QVERIFY(de.value_in_range_in_instance("supply_2/voltage"));

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("supply_probe_count/voltage", 5.0);, DataEngineErrorNumber::instance_count_yet_undefined);

    QVERIFY(!de.is_complete());

    QVERIFY_EXCEPTION_THROWN_error_number(de.use_instance("supply_probe_count", "probe 1", 1);, DataEngineErrorNumber::instance_count_yet_undefined);

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_instance_count("probe_count_fail", 3);, DataEngineErrorNumber::instance_count_does_not_exist);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_instance_count("probe_count", 0);
                                          , DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative);

    de.set_instance_count("probe_count", 3);

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_instance_count("probe_count", 4);, DataEngineErrorNumber::instance_count_already_defined);

    de.set_actual_number("supply_probe_count/voltage", 4.9);

    QVERIFY_EXCEPTION_THROWN_error_number(de.use_instance("supply_probe_count", "Probe abc", 4);, DataEngineErrorNumber::instance_count_exceeding);

    de.use_instance("supply_probe_count", "Probe abc", 2);

    de.set_actual_number("supply_probe_count/voltage", 5.1);
    de.use_instance("supply_probe_count", "Probe xyz", 3);
    //wait with filling for checking if not complete

    QVERIFY(de.value_complete_in_instance("supply_2/voltage"));
    QVERIFY(!de.value_complete("supply_2/voltage"));
    de.use_instance("supply_2", "Instanz 2", 2);
    de.set_actual_number("supply_2/voltage", 4.0); //should be ok, but fail later since actual value is out of range
    QVERIFY(de.value_complete_in_instance("supply_2/voltage"));
    QVERIFY(de.value_complete("supply_2/voltage"));
    QVERIFY(!de.is_complete());
    de.set_actual_number("supply_probe_count/voltage", 5.0);

    de.set_actual_number("supply_varants/voltage", 5.0);

    de.use_instance("supply_varants", "Probe B", 2);
    de.set_actual_number("supply_varants/voltage", 5.0);

    de.use_instance("supply_varants", "Probe C", 3);
    de.set_actual_number("supply_varants/voltage", 5.0);

    de.set_actual_number("supply_2_object/voltage", 5.0);

    de.use_instance("supply_probe_count", "", 1);
    QCOMPARE(de.get_actual_value("supply_probe_count/voltage"), QString("4900 mV"));
    de.use_instance("supply_probe_count", "", 2);
    QCOMPARE(de.get_actual_value("supply_probe_count/voltage"), QString("5100 mV"));
    de.use_instance("supply_probe_count", "", 3);
    QCOMPARE(de.get_actual_value("supply_probe_count/voltage"), QString("5000 mV"));

    de.use_instance("supply_2_object", "Probe B", 2);
    de.set_actual_number("supply_2_object/voltage", 5.0);

    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());
    de.set_actual_number("supply_2/voltage", 5.0);

    QVERIFY(de.all_values_in_range());

#endif
}

void Test_Data_engine::test_instances_with_different_variants_and_wrong_instance_size() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{

    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_instance_count("probe_count", 3);, DataEngineErrorNumber::instance_count_must_match_list_of_dependency_values);

#endif
}
void Test_Data_engine::test_instances_with_different_variants_and_wrong_instance_size2() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{

    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true,
                     "other_tag": 0
                },
                "data":[
                    {	"name": "voltage",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false,
                    "other_tag": 0
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    tags.insert("other_tag", {0, 1, 2});
    Data_engine de{input, tags};

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_instance_count("probe_count", 2);, DataEngineErrorNumber::list_of_dependency_values_must_be_of_equal_length);

#endif
}

void Test_Data_engine::test_exceptional_approval() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{

    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    tags.insert("dummy", {true});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_instance_count("probe_count", 2);
    de.set_actual_number("supply_variants/voltage", 5.0);
    de.use_instance("supply_variants", "Probe B", 2);
    de.set_actual_number("supply_variants/voltage", 6.0);

    de.use_instance("supply_variants", "Probe A", 1);
    de.use_instance("supply_variants", "Probe B", 2);

    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());

    // auto filename = QSettings{}.value(Globals::path_to_excpetional_approval_db_key, "").toString();
    QString filename = QString{"../../tests/scripts/sonderfreigaben.json"};
    //  qDebug() << QDir::currentPath();
    ExceptionalApprovalDB ea = ExceptionalApprovalDB{filename};
    de.do_exceptional_approvals(ea, nullptr);
    auto ff = ea.get_failed_fields();
    QCOMPARE(ff.count(), 2);
#endif
}

void Test_Data_engine::test_instances_with_different_variants() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{

    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    tags.insert("dummy", {true});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_instance_count("probe_count", 2);

    de.set_actual_number("supply_variants/voltage", 4.0);

    de.use_instance("supply_variants", "Probe B", 2);
    de.set_actual_number("supply_variants/voltage", 3.0);

    de.use_instance("supply_variants", "Probe B", 1);
    QCOMPARE(de.get_actual_value("supply_variants/voltage"), QString("4000 mV"));
    de.use_instance("supply_variants", "Probe B", 2);
    QCOMPARE(de.get_actual_value("supply_variants/voltage"), QString("3000 mV"));

    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());
    de.set_actual_number("supply_variants/voltage", 5.0);
    QVERIFY(de.all_values_in_range());
#endif
}

void Test_Data_engine::test_instances_with_different_variants_and_references_fail1() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_number_ref",          "value": "[supply_variants/voltage.desired]",    "tolerance": 20,        	"nice_name": "Referenz zum bool AO ist"         }

        ]
    },
    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("referenzen/test_number_ref", 2);
                                          , DataEngineErrorNumber::reference_cant_be_used_because_its_pointing_to_a_yet_undefined_instance);

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_instance_count("probe_count", 2);
                                          , DataEngineErrorNumber::reference_pointing_to_multiinstance_with_different_values);

#endif
}

void Test_Data_engine::test_instances_with_different_variants_and_references_and_different_signatures() {
#if !DISABLE_ALL || 1

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_number_ref",          "value": "[supply_variants/voltageB.desired]",    "tolerance": 20,        	"nice_name": "Referenz zum bool AO ist"         },
            {	"name": "test_number_ref_tolerance_inherited",          "value": "[supply_variants/voltageA.desired]",    "tolerance": "[inherited]",        	"nice_name": "[inherited]"         }

        ]
    },
    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltageA",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltageB",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("referenzen/test_number_ref", 5000);
                                          , DataEngineErrorNumber::reference_cant_be_used_because_its_pointing_to_a_yet_undefined_instance);

    de.set_instance_count("probe_count", 2);

    de.set_actual_number("referenzen/test_number_ref", 5);
    de.set_actual_number("supply_variants/voltageA", 4);

    de.set_actual_number("referenzen/test_number_ref_tolerance_inherited", 4.21);
    QVERIFY(!de.value_in_range("referenzen/test_number_ref_tolerance_inherited"));
    de.set_actual_number("referenzen/test_number_ref_tolerance_inherited", 4.0);
    QVERIFY(de.value_in_range("referenzen/test_number_ref_tolerance_inherited"));
    de.set_actual_number("referenzen/test_number_ref_tolerance_inherited", 4.199);
    QVERIFY(de.value_in_range("referenzen/test_number_ref_tolerance_inherited"));
    QCOMPARE(de.get_description("referenzen/test_number_ref_tolerance_inherited"), QString("Variable Instances. Two Variants. This variant is chosen"));

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("supply_variants/voltageB", 4);, DataEngineErrorNumber::no_field_id_found);

    de.use_instance("supply_variants", "Probe B", 2);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("supply_variants/voltageA", 4);, DataEngineErrorNumber::no_field_id_found);

    de.set_actual_number("supply_variants/voltageB", 5);

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_value("supply_variants/voltageA"), DataEngineErrorNumber::no_field_id_found);
    de.use_instance("supply_variants", "", 1);
    QCOMPARE(de.get_actual_value("supply_variants/voltageA"), QString("4000 mV"));

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_value("supply_variants/voltageB"), DataEngineErrorNumber::no_field_id_found);
    de.use_instance("supply_variants", "", 2);
    QCOMPARE(de.get_actual_value("supply_variants/voltageB"), QString("5000 mV"));

    QVERIFY(de.all_values_in_range());

#endif
}

void Test_Data_engine::test_instances_with_different_variants_and_references_and_different_signatures_and_already_defined_instance_counts() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_number_ref",          "value": "[supply_variants/voltageB.desired]",    "tolerance": 20,        	"nice_name": "Referenz zum bool AO ist"         }

        ]
    },
    "supply_variants":{
        "instance_count": "2",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltageA",	 	"value": 4000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltageB",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.section_uses_variants("referenzen"));
    QVERIFY(de.section_uses_variants("supply_variants"));

    de.set_actual_number("referenzen/test_number_ref", 5);

    de.set_actual_number("supply_variants/voltageA", 4);

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("supply_variants/voltageB", 4);, DataEngineErrorNumber::no_field_id_found);

    de.use_instance("supply_variants", "Probe B", 2);
    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("supply_variants/voltageA", 4);, DataEngineErrorNumber::no_field_id_found);

    de.set_actual_number("supply_variants/voltageB", 5);

    de.use_instance("supply_variants", "Probe B", 1);
    QCOMPARE(de.get_actual_value("supply_variants/voltageA"), QString("4000 mV"));

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_value("supply_variants/voltageB");, DataEngineErrorNumber::no_field_id_found);

    de.use_instance("supply_variants", "Probe B", 2);
    QCOMPARE(de.get_actual_value("supply_variants/voltageB"), QString("5000 mV"));

    QVERIFY(de.all_values_in_range());

#endif
}

void Test_Data_engine::test_instances_with_different_variants_and_references_equal_targets() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_number_ref",          "value": "[supply_variants/voltage.desired]",    "tolerance": 20,        	"nice_name": "Referenz zum bool AO ist"         }

        ]
    },
    "supply_variants":{
        "instance_count": "probe_count",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY_EXCEPTION_THROWN_error_number(de.set_actual_number("referenzen/test_number_ref", 5000);
                                          , DataEngineErrorNumber::reference_cant_be_used_because_its_pointing_to_a_yet_undefined_instance);

    de.set_instance_count("probe_count", 2);
    de.set_actual_number("referenzen/test_number_ref", 5);

    de.set_actual_number("supply_variants/voltage", 5);
    de.use_instance("supply_variants", "Probe B", 2);
    de.set_actual_number("supply_variants/voltage", 5);

    QVERIFY(de.all_values_in_range());

#endif
}

void Test_Data_engine::test_instances_with_different_variants_and_references_readily_initialized() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "test_number_ref",          "value": "[supply_variants/voltage.desired]",    "tolerance": 20,        	"nice_name": "Referenz zum bool AO ist"         }

        ]
    },
    "supply_variants":{
        "instance_count": "2",
        "variants":[
            {
                "apply_if": {
                    "is_good_variant": true
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen"	}
                ]
            },
            {
                "apply_if": {
                    "is_good_variant": false
                },
                "data":[
                    {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Variable Instances. Two Variants. This variant is chosen aswell"	}
                ]
            }
        ]
    }
}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    tags.insert("is_good_variant", {true, false});
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_actual_number("referenzen/test_number_ref", 5);

    de.set_actual_number("supply_variants/voltage", 5);
    de.use_instance("supply_variants", "Probe B", 2);
    de.set_actual_number("supply_variants/voltage", 5);

    QVERIFY(de.all_values_in_range());

#endif
}

void Test_Data_engine::test_instances_bool_string() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{

    "supply_2":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage_bool",	 	"value": true,      "nice_name": "Two Instances. bool"	},
            {	"name": "voltage_string",	 	"value": "test123",	"nice_name": "Two Instances. string"	}
        ]
    }

}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_actual_bool("supply_2/voltage_bool", true);
    de.set_actual_text("supply_2/voltage_string", "test123");

    QVERIFY(!de.is_complete());

    QVERIFY(!de.value_in_range("supply_2/voltage_bool"));
    QVERIFY(!de.value_in_range("supply_2/voltage_string"));

    de.use_instance("supply_2", "Probe C", 2);

    de.set_actual_bool("supply_2/voltage_bool", false);
    de.set_actual_text("supply_2/voltage_string", "test124");

    QVERIFY(!de.value_in_range("supply_2/voltage_bool"));
    QVERIFY(!de.value_in_range("supply_2/voltage_string"));

    de.use_instance("supply_2", "Probe C", 1);
    QCOMPARE(de.get_actual_value("supply_2/voltage_bool"), QString("Yes"));
    QCOMPARE(de.get_actual_value("supply_2/voltage_string"), QString("test123"));
    de.use_instance("supply_2", "Probe C", 2);
    QCOMPARE(de.get_actual_value("supply_2/voltage_bool"), QString("No"));
    QCOMPARE(de.get_actual_value("supply_2/voltage_string"), QString("test124"));

    QVERIFY(de.is_complete());
    QVERIFY(!de.all_values_in_range());

    de.set_actual_bool("supply_2/voltage_bool", true);
    de.set_actual_text("supply_2/voltage_string", "test123");

    QVERIFY(de.value_in_range("supply_2/voltage_bool"));
    QVERIFY(de.value_in_range("supply_2/voltage_string"));

    QVERIFY(de.all_values_in_range());
    QVERIFY(de.is_complete());

#endif
}

void Test_Data_engine::test_faulty_instancecount() {
#if !DISABLE_ALL || 0

    std::stringstream input_zero{R"(
{

    "supply_0":{
        "instance_count": 0,
        "data":[
            {	"name": "voltage_bool",	 	"value": true,      "nice_name": "Two Instances. bool"	},
            {	"name": "voltage_string",	 	"value": "test123",	"nice_name": "Two Instances. string"	}
        ]
    }

}
                            )"};

    std::stringstream input_fraction{R"(
{

    "supply_0":{
        "instance_count": 1.5,
        "data":[
            {	"name": "voltage_bool",	 	"value": true,      "nice_name": "Two Instances. bool"	},
            {	"name": "voltage_string",	 	"value": "test123",	"nice_name": "Two Instances. string"	}
        ]
    }

}
                            )"};

    std::stringstream input_negative{R"(
{

    "supply_0":{
        "instance_count": -1,
        "data":[
            {	"name": "voltage_bool",	 	"value": true,      "nice_name": "Two Instances. bool"	},
            {	"name": "voltage_string",	 	"value": "test123",	"nice_name": "Two Instances. string"	}
        ]
    }

}
                            )"};
    QMap<QString, QList<QVariant>> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_zero, tags);, DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_fraction, tags);, DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_negative, tags);, DataEngineErrorNumber::instance_count_must_not_be_zero_nor_fraction_nor_negative);

#endif
}

void Test_Data_engine::test_if_exception_when_instance_count_defined_within_variant() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "supply-voltage":[
        {
            "apply_if":{
                "sound":"[1.42-1.442]"
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        },
        {
            "instance_count":2,
            "apply_if":{
                "sound":"[1.43-*]"
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    ]
}
                            )"};

    QMap<QString, QList<QVariant>> tags;
    tags.insert("sound", {1.42});
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::instance_count_must_not_be_defined_in_variant_scope);

#endif
}

void Test_Data_engine::test_if_exception_when_same_fields_across_variants_with_different_types_1() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "supply-voltage":[
        {
            "apply_if":{
                "sound":"[1.42-1.442]"
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        },
        {
            "apply_if":{
                "sound":"[1.43-*]"
            },
            "data":[
                {	"name": "Voltage",	 	"value": "5000",		"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    ]
}
                            )"};

    QMap<QString, QList<QVariant>> tags;
    tags.insert("sound", {1.42});
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input, tags);, DataEngineErrorNumber::inconsistant_types_across_variants);

#endif
}

void Test_Data_engine::test_if_exception_when_same_fields_across_variants_with_different_types_2() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "supply-voltage":[
        {
            "apply_if":{
                "sound":"[1.42-1.442]"
            },
            "data":[
                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
            ]
        },
        {
            "apply_if":{
                "sound":"[1.43-*]"
            },
            "data":[
                {	"name": "Voltage",	 	"value": "5000",		"nice_name": "Betriebsspannung +5V"	}
            ]
        }
    ]
}
                            )"};

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine{input};, DataEngineErrorNumber::inconsistant_types_across_variants);

#endif
}

void Test_Data_engine::test_if_exception_when_same_fields_across_variants_with_different_types_using_references() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "voltage_source",       "value": "[test_valuesA/voltage_source.actual,  test_valuesB/voltage_blue.actual]",                      	"nice_name": "Spannungsvergleich"       },
            {	"name": "voltage_color_ist",    "value": "[test_valuesB/voltage_green.actual,   test_valuesB/voltage_red.actual]",       "tolerance": 10,   "nice_name": "Farbige Spannung ist"     }
        ]
    },

    "test_valuesA":{
        "data":[
            {	"name": "voltage_source",       "type": "bool",                    "nice_name": "text just for printing"	}
        ]
    },

    "test_valuesB":[
        {
            "apply_if":{
                "color":"blue"
            },
            "data":[
               {	"name": "voltage_blue",     "value": "tex_value",    "nice_name": "Spannung blau"            }
            ]
        },
        {
            "apply_if":{
                "color":"green"
            },
            "data":[
                {	"name": "voltage_green",    "value": 50,    "tolerance": 10,	"unit": "mV",   "si_prefix": 1e-3,  "nice_name": "Spannung gruen"           }
            ]
        },
        {
            "apply_if":{
                "color":"*"
            },
            "data":[
                {	"name": "voltage_red",    "value": 500,    "tolerance": 10,	"unit": "mV",   "si_prefix": 1e-3,  "nice_name": "Spannung rot"           }
            ]
        }
    ]

}
                            )"};
    // Data_engine{input};
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine{input};, DataEngineErrorNumber::inconsistant_types_across_variants_and_reference_targets);
#endif
}

void Test_Data_engine::test_instances_with_references() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
{

    "references":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage_ref_bool",	 	"value": "[values/voltage_bool.desired]",                       "nice_name": "Two Instances. reference -> bool"     },
            {	"name": "voltage_ref_string",   "value": "[values/voltage_string.desired]",                     "nice_name": "Two Instances. reference -> string"	},
            {	"name": "voltage_ref_number",   "value": "[values/voltage_number.desired]",    "tolerance":20,  "nice_name": "Two Instances. reference -> number"	}
        ]
    },
    "values":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage_bool",	 	"value": true,                      "nice_name": "Two Instances. bool"      },
            {	"name": "voltage_string",	"value": "test123",                 "nice_name": "Two Instances. string"	},
            {	"name": "voltage_number",	"value": 500,       "tolerance":10,	"nice_name": "Two Instances. number"	}
        ]
    }

}
                            )"};

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_number("values/voltage_string"), DataEngineErrorNumber::desired_value_is_not_a_number);
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_desired_number("references/voltage_ref_bool"), DataEngineErrorNumber::desired_value_is_not_a_number);

    QCOMPARE(de.get_desired_number("references/voltage_ref_number"), 500.0);

    de.set_actual_bool("references/voltage_ref_bool", true);
    de.set_actual_text("references/voltage_ref_string", "test123");
    de.set_actual_number("references/voltage_ref_number", 520);

    QVERIFY(!de.is_complete());

    QVERIFY(!de.value_in_range("references/voltage_ref_bool"));
    QVERIFY(!de.value_in_range("references/voltage_ref_string"));
    QVERIFY(!de.value_in_range("references/voltage_ref_number"));

    de.use_instance("references", "Probe C", 2);

    de.set_actual_bool("references/voltage_ref_bool", true);
    de.set_actual_text("references/voltage_ref_string", "test123");
    de.set_actual_number("references/voltage_ref_number", 530);

    QVERIFY(de.value_in_range("references/voltage_ref_bool"));
    QVERIFY(de.value_in_range("references/voltage_ref_string"));
    QVERIFY(!de.value_in_range("references/voltage_ref_number"));

    de.set_actual_number("references/voltage_ref_number", 510);

    QVERIFY(de.value_in_range("references/voltage_ref_number"));

    de.use_instance("references", "Probe C", 1);
    QCOMPARE(de.get_actual_value("references/voltage_ref_bool"), QString("Yes"));
    QCOMPARE(de.get_actual_value("references/voltage_ref_string"), QString("test123"));
    QCOMPARE(de.get_actual_value("references/voltage_ref_number"), QString("520"));

    de.use_instance("references", "Probe C", 2);
    QCOMPARE(de.get_actual_value("references/voltage_ref_bool"), QString("Yes"));
    QCOMPARE(de.get_actual_value("references/voltage_ref_string"), QString("test123"));
    QCOMPARE(de.get_actual_value("references/voltage_ref_number"), QString("510"));

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_number("references/voltage_ref_bool");, DataEngineErrorNumber::actual_value_is_not_a_number);
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_number("references/voltage_ref_string");, DataEngineErrorNumber::actual_value_is_not_a_number);
    QCOMPARE(de.get_actual_number("references/voltage_ref_number"), 510.0);

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_number("values/voltage_number");, DataEngineErrorNumber::actual_value_not_set);

    de.set_actual_bool("values/voltage_bool", true);
    de.set_actual_text("values/voltage_string", "test123");
    de.set_actual_number("values/voltage_number", 510);

    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_number("values/voltage_bool");, DataEngineErrorNumber::actual_value_is_not_a_number);
    QVERIFY_EXCEPTION_THROWN_error_number(de.get_actual_number("values/voltage_string");, DataEngineErrorNumber::actual_value_is_not_a_number);
    QCOMPARE(de.get_actual_number("values/voltage_number"), 510.0);

    QVERIFY(!de.is_complete());

    QVERIFY(!de.value_in_range("values/voltage_bool"));
    QVERIFY(!de.value_in_range("values/voltage_string"));
    QVERIFY(!de.value_in_range("values/voltage_number"));

    de.use_instance("values", "Probe C", 2);

    de.set_actual_bool("values/voltage_bool", true);
    de.set_actual_text("values/voltage_string", "test123");
    de.set_actual_number("values/voltage_number", 505);

    QVERIFY(de.value_in_range("values/voltage_bool"));
    QVERIFY(de.value_in_range("values/voltage_string"));
    QVERIFY(de.value_in_range("values/voltage_number"));

    QVERIFY(de.all_values_in_range());
    QVERIFY(de.is_complete());

#endif
}

void Test_Data_engine::test_instances_with_references_to_multiinstance_actual_value() {
#if !DISABLE_ALL || 0

    std::stringstream input_bool{R"(
{

    "references":{
        "data":[
            {	"name": "voltage_ref_bool",	 	"value": "[values/voltage_bool.actual]",                       "nice_name": "Two Instances. reference -> bool"     }
        ]
    },
    "values":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage_bool",	 	"value": true,                      "nice_name": "Two Instances. bool"      }
        ]
    }

}
                            )"};

    std::stringstream input_string{R"(
{

    "references":{
        "data":[
            {	"name": "voltage_ref_string",   "value": "[values/voltage_string.actual]",                     "nice_name": "Two Instances. reference -> string"	}
        ]
    },
    "values":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage_string",	"value": "test123",                 "nice_name": "Two Instances. string"	}
        ]
    }

}
                            )"};

    std::stringstream input_number{R"(
{

    "references":{
        "data":[
            {	"name": "voltage_ref_number",   "value": "[values/voltage_number.actual]",    "tolerance":20,  "nice_name": "Two Instances. reference -> number"	}
        ]
    },
    "values":{
        "instance_count": 2,
        "data":[
            {	"name": "voltage_number",	"value": 500,       "tolerance":10,	"nice_name": "Two Instances. number"	}
        ]
    }

}
                            )"};

    QMap<QString, QList<QVariant>> tags;
    Data_engine de_bool(input_bool, tags);
    Data_engine de_str(input_string, tags);
    Data_engine de_num(input_number, tags);

    QVERIFY_EXCEPTION_THROWN_error_number(de_bool.value_in_range("references/voltage_ref_bool"),
                                          DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);
    QVERIFY_EXCEPTION_THROWN_error_number(de_str.value_in_range("references/voltage_ref_string"),
                                          DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);
    QVERIFY_EXCEPTION_THROWN_error_number(de_num.value_in_range("references/voltage_ref_number"),
                                          DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);

    QVERIFY_EXCEPTION_THROWN_error_number(de_bool.get_desired_value_as_string("references/voltage_ref_bool"),
                                          DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);
    QVERIFY_EXCEPTION_THROWN_error_number(de_str.get_desired_value_as_string("references/voltage_ref_string"),
                                          DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);
    QVERIFY_EXCEPTION_THROWN_error_number(de_num.get_desired_value_as_string("references/voltage_ref_number"),
                                          DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);

    QVERIFY_EXCEPTION_THROWN_error_number(de_bool.all_values_in_range(), DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);
    QVERIFY_EXCEPTION_THROWN_error_number(de_str.all_values_in_range(), DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);
    QVERIFY_EXCEPTION_THROWN_error_number(de_num.all_values_in_range(), DataEngineErrorNumber::reference_must_not_point_to_multiinstance_actual_value);

#endif
}

void Test_Data_engine::test_instances_with_strange_types() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{

    "values":{
        "instance_count": {},
        "data":[
            {	"name": "voltage_bool",	 	"value": true,                      "nice_name": "Two Instances. bool"      }
        ]
    }

}
                            )"};

    QMap<QString, QList<QVariant>> tags;

    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine de_bool(input, tags);, DataEngineErrorNumber::wrong_type_for_instance_count);

#endif
}
void Test_Data_engine::test_empty_entries() {
#if !DISABLE_ALL || 0

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
    QMap<QString, QList<QVariant>> tags;
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
    QMap<QString, QList<QVariant>> tags;

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
    QMap<QString, QList<QVariant>> tags;
    Data_engine(input, tags);
#endif
}

void Test_Data_engine::test_reference_date_time() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
{
    "referenzen":{
        "data":[
            {	"name": "ref_today",       "value": "[test_valuesA/today.actual]", 		"nice_name": "Spannungsvergleich"       }
        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "today",       "type": "datetime",                  "nice_name": "text just for printing"	}
        ]
    }

}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine de(input, tags);, DataEngineErrorNumber::datetime_dont_support_references_yet);

#endif
}

void Test_Data_engine::test_references() {
#if !DISABLE_ALL || 0

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
    QMap<QString, QList<QVariant>> tags;
    tags.insert("color", {"green"});
    Data_engine de{input, tags};
    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());
    QVERIFY(!de.value_in_range("referenzen/voltage_source"));

    de.set_actual_number("test_valuesA/voltage_source", 0.5);
    QVERIFY(de.value_in_range("test_valuesA/voltage_source"));
    QVERIFY(!de.value_in_range("referenzen/voltage_source"));
    de.set_actual_number("referenzen/voltage_source", 0.506);
    QVERIFY(!de.value_in_range("referenzen/voltage_source"));
    de.set_actual_number("referenzen/voltage_source", 0.505);

    QVERIFY(de.value_in_range("referenzen/voltage_source"));

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist"));
    de.set_actual_number("referenzen/voltage_color_ist", 0.300);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist"));

    de.set_actual_number("test_valuesB/voltage_green", 0.306);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_ist"));

    de.set_actual_number("test_valuesB/voltage_green", 0.305);
    QVERIFY(de.value_in_range("referenzen/voltage_color_ist"));

    QVERIFY(!de.is_complete());
    QVERIFY(!de.all_values_in_range());

    QVERIFY(!de.value_in_range("referenzen/voltage_color_soll"));
    de.set_actual_number("referenzen/voltage_color_soll", 0.056);
    QVERIFY(!de.value_in_range("referenzen/voltage_color_soll"));

    de.set_actual_number("referenzen/voltage_color_soll", 0.055);
    QVERIFY(de.value_in_range("referenzen/voltage_color_soll"));

    de.set_actual_number("test_valuesB/voltage_green", 0.060);
    de.set_actual_number("referenzen/voltage_color_ist", 0.065);

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
    QMap<QString, QList<QVariant>> tags;
    tags.insert("color", {"green"});

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
    QMap<QString, QList<QVariant>> tags;
    tags.insert("color", {"green"});
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
    QMap<QString, QList<QVariant>> tags;
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

            {	"name": "test_number_ref",          "value": "[test_valuesA/test_number.desired]",          "tolerance":2 ,         "nice_name": "Referenz zum numer soll"  },
            {	"name": "test_bool_ref",            "value": "[test_valuesA/test_bool.desired]",                                    "nice_name": "Referenz zum bool soll"   },
            {	"name": "test_string_ref",          "value": "[test_valuesA/test_string.desired]",                                  "nice_name": "Referenz zum string soll" }
        ]
    },
    "test_valuesA":{
        "data":[
            {	"name": "test_number",                  "value": 100,           "tolerance":1 ,   "unit":"mA",   "si_prefix":1e-3,          "nice_name": "Original number"  },
            {	"name": "test_bool",                    "value": true,                                                                      "nice_name": "Original bool"    },
            {	"name": "test_string",                  "value": "test_string",                                                             "nice_name": "Original string"  }
        ]
    }

}
                            )"};
    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    QCOMPARE(de.value_in_range("test_valuesA/test_number"), false);
    QCOMPARE(de.value_in_range("test_valuesA/test_bool"), false);
    QCOMPARE(de.value_in_range("test_valuesA/test_string"), false);

    QCOMPARE(de.value_in_range("referenzen/test_number_ref"), false);
    QCOMPARE(de.value_in_range("referenzen/test_bool_ref"), false);
    QCOMPARE(de.value_in_range("referenzen/test_string_ref"), false);

    QCOMPARE(de.is_desired_value_set("referenzen/test_string_ref"), true);
    QCOMPARE(de.is_desired_value_set("test_valuesA/test_number"), true);
    QCOMPARE(de.is_desired_value_set("test_valuesA/test_bool"), true);
    QCOMPARE(de.is_desired_value_set("test_valuesA/test_string"), true);

    QCOMPARE(de.is_number("test_valuesA/test_string"), false);
    QCOMPARE(de.is_number("test_valuesA/test_number"), true);
    QCOMPARE(de.is_number("referenzen/test_number_ref"), true);

    QCOMPARE(de.is_bool("test_valuesA/test_string"), false);
    QCOMPARE(de.is_bool("test_valuesA/test_bool"), true);
    QCOMPARE(de.is_bool("referenzen/test_bool_ref"), true);

    QCOMPARE(de.is_text("test_valuesA/test_bool"), false);
    QCOMPARE(de.is_text("test_valuesA/test_string"), true);
    QCOMPARE(de.is_text("referenzen/test_string_ref"), true);

    de.set_actual_number("test_valuesA/test_number", 0.101);
    de.set_actual_bool("test_valuesA/test_bool", true);
    de.set_actual_text("test_valuesA/test_string", "test_string");

    de.set_actual_number("referenzen/test_number_ref", 0.102);
    de.set_actual_bool("referenzen/test_bool_ref", true);
    de.set_actual_text("referenzen/test_string_ref", "test_string");

    QCOMPARE(de.value_in_range("test_valuesA/test_number"), true);
    QCOMPARE(de.value_in_range("test_valuesA/test_bool"), true);
    QCOMPARE(de.value_in_range("test_valuesA/test_string"), true);

    QCOMPARE(de.value_in_range("referenzen/test_number_ref"), true);
    QCOMPARE(de.value_in_range("referenzen/test_bool_ref"), true);
    QCOMPARE(de.value_in_range("referenzen/test_string_ref"), true);

    de.set_actual_text("test_valuesA/test_string", "TEST321");
    de.set_actual_text("referenzen/test_string_ref", "TEST123");

    QCOMPARE(de.get_actual_value("test_valuesA/test_number"), QString("101 mA"));
    QCOMPARE(de.get_description("test_valuesA/test_number"), QString("Original number"));
    QCOMPARE(de.get_desired_value_as_string("test_valuesA/test_number"), QString("100 (±1) mA"));
    QCOMPARE(de.get_unit("test_valuesA/test_number"), QString("mA"));

    QCOMPARE(de.get_entry_type("test_valuesA/test_number").t, EntryType::Number);
    QCOMPARE(de.get_entry_type("test_valuesA/test_bool").t, EntryType::Bool);
    QCOMPARE(de.get_entry_type("test_valuesA/test_string").t, EntryType::Text);

    QCOMPARE(de.get_entry_type("referenzen/test_number_ref").t, EntryType::Number);
    QCOMPARE(de.get_entry_type("referenzen/test_bool_ref").t, EntryType::Bool);
    QCOMPARE(de.get_entry_type("referenzen/test_string_ref").t, EntryType::Text);

    QCOMPARE(de.get_si_prefix("referenzen/test_number_ref"), 0.001);
    QCOMPARE(de.get_si_prefix("test_valuesA/test_number"), 0.001);
    QCOMPARE(de.get_si_prefix("test_valuesA/test_bool"), 1.0);
    QCOMPARE(de.get_si_prefix("test_valuesA/test_string"), 1.0);

    QCOMPARE(de.get_actual_value("test_valuesA/test_bool"), QString("Yes"));
    QCOMPARE(de.get_description("test_valuesA/test_bool"), QString("Original bool"));
    QCOMPARE(de.get_desired_value_as_string("test_valuesA/test_bool"), QString("Yes"));
    QCOMPARE(de.get_unit("test_valuesA/test_bool"), QString(""));

    QCOMPARE(de.get_actual_value("test_valuesA/test_string"), QString("TEST321"));
    QCOMPARE(de.get_description("test_valuesA/test_string"), QString("Original string"));
    QCOMPARE(de.get_desired_value_as_string("test_valuesA/test_string"), QString("test_string"));
    QCOMPARE(de.get_unit("test_valuesA/test_string"), QString(""));

    QCOMPARE(de.get_actual_value("referenzen/test_number_ref"), QString("102 mA"));
    QCOMPARE(de.get_description("referenzen/test_number_ref"), QString("Referenz zum numer soll"));
    QCOMPARE(de.get_desired_value_as_string("referenzen/test_number_ref"), QString("100 (±2) mA"));
    QCOMPARE(de.get_unit("referenzen/test_number_ref"), QString("mA"));

    QCOMPARE(de.get_actual_value("referenzen/test_bool_ref"), QString("Yes"));
    QCOMPARE(de.get_description("referenzen/test_bool_ref"), QString("Referenz zum bool soll"));
    QCOMPARE(de.get_desired_value_as_string("referenzen/test_bool_ref"), QString("Yes"));
    QCOMPARE(de.get_unit("referenzen/test_bool_ref"), QString(""));

    QCOMPARE(de.get_actual_value("referenzen/test_string_ref"), QString("TEST123"));
    QCOMPARE(de.get_description("referenzen/test_string_ref"), QString("Referenz zum string soll"));
    QCOMPARE(de.get_desired_value_as_string("referenzen/test_string_ref"), QString("test_string"));
    QCOMPARE(de.get_unit("referenzen/test_string_ref"), QString(""));

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
    QMap<QString, QList<QVariant>> tags;
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

    QCOMPARE(de.is_desired_value_set("test_valuesA/test_string_actual_only"), false);
    QCOMPARE(de.is_desired_value_set("test_valuesA/test_bool"), true);

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
    QMap<QString, QList<QVariant>> tags;
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

void Test_Data_engine::test_get_possible_variant_tag_values() {
#if !DISABLE_ALL || 0

    std::stringstream input{R"(
                            {
                                "A":{
                                    "instance_count":"count_name_a",
                                    "variants":[
                                        {
                                            "apply_if":{
                                                "high_power_edidtion":true
                                            },
                                            "data":[
                                                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                                            ]
                                        },
                                        {
                                            "apply_if":{
                                                "high_power_edidtion":true
                                            },
                                            "data":[
                                                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                                            ]
                                        }
                                    ]
                                },
                                "B":{
                                    "instance_count":"count_name_a",
                                    "variants":[
                                        {
                                            "apply_if":{
                                                "high_power_edidtion":true
                                            },
                                            "data":[
                                                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                                            ]
                                        },
                                        {
                                            "apply_if":{

                                                "high_power_edidtion":true
                                            },
                                            "data":[
                                                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                                            ]
                                        }
                                    ]
                                },

                                "C":{
                                    "instance_count":"count_name_c",
                                    "variants":[
                                        {
                                            "apply_if":{
                                                "high_power_edidtion":true
                                            },
                                            "data":[
                                                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                                            ]
                                        },
                                        {
                                            "apply_if":{
                                                "high_power_edidtion":true
                                            },
                                            "data":[
                                                {	"name": "voltage",	 	"value": 5000,	"tolerance": 200,	"unit": "mV", "si_prefix": 1e-3,	"nice_name": "Betriebsspannung +5V"	}
                                            ]
                                        }
                                    ]
                                }
                            }
                            )"};

    Data_engine de{input};
    QStringList count_names = de.get_instance_count_names();
    QCOMPARE(count_names.count(), 2);
    QCOMPARE(count_names[0], QString("count_name_a"));
    QCOMPARE(count_names[1], QString("count_name_c"));

    de.set_instance_count(count_names[0], 2);
    de.set_instance_count(count_names[1], 2);

    de.fill_engine_with_dummy_data();
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
    QMap<QString, QList<QVariant>> tags;
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
    QMap<QString, QList<QVariant>> tags;

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
    QMap<QString, QList<QVariant>> tags;
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
    QMap<QString, QList<QVariant>> tags;

    Data_engine(input_ok, tags);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_fail_unit, tags);, DataEngineErrorNumber::invalid_data_entry_key);
    QVERIFY_EXCEPTION_THROWN_error_number(Data_engine(input_fail_prefix, tags);, DataEngineErrorNumber::invalid_data_entry_key);
#endif
}

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "../src/Windows/mainwindow.h"

void Test_Data_engine::test_preview() {
    MainWindow::gui_thread = QThread::currentThread(); //required to make MainWindow::await_execute_in_gui_thread work which is required by generate_pdf
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
		{
			"testA":{
				"data":[
					{	"name": "idA1",	 	"value": 100,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idA2",	 	"value": 200,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	}
				]
			},
			"testB":{
				"data":[
					{	"name": "idB1",	 	"value": 300,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idB2",	 	"value": 400,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idB3",	 	"value": 500,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idB4",	 	"value": 600,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	}
				]
			}
		}
	)"};

#if 1
    QString db_name = "";
    QTemporaryFile db_file;
    if (db_file.open()) {
        db_name = db_file.fileName(); // returns the unique file name
                                      //The file name of the temporary file can be found by calling fileName().
                                      //Note that this is only defined after the file is first opened; the
                                      //function returns an empty string before this.
    }
    db_file.close(); //Reopening a QTemporaryFile after calling close() is safe
                     // QFile::remove(db_name);
                     // assert(QFile{db_name}.exists() == false);
#endif
    QSqlDatabase db;
    if (QSqlDatabase::contains()) {
        db = QSqlDatabase::database();
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE");
    }

    //  qDebug() << db_name;
    db.setDatabaseName(db_name);
    bool opend = db.open();
    if (!opend) {
        qDebug() << "SQLConnection error: " << db.lastError().text();
    }

    QVERIFY(opend);

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};

    int argc = 1;
    char executable[] = "";
    char *executable2 = executable;
    char **argv = &executable2;
    QApplication app(argc, argv);

    de.set_actual_number("testA/idA1", 100);
    de.set_actual_number("testA/idA2", 200);
    de.set_actual_number("testB/idB1", 300);
    de.set_actual_number("testB/idB2", 400);
    de.set_actual_number("testB/idB3", 500);
    de.set_actual_number("testB/idB4", 100);
    //QVERIFY(de.all_values_in_range());
    QVERIFY(de.is_complete());
    de.fill_database(db);
    const QString report_title = "Report Title";
    const QString image_footer_path = "";
    const QString image_header_path = "";
    QList<PrintOrderItem> print_order;
    de.generate_template("testreport.lrxml", db_name, report_title, image_footer_path, image_header_path, "", "", "", "", "", "", print_order);
    QVERIFY(de.generate_pdf("testreport.lrxml", QDir::current().absoluteFilePath("test.pdf").toStdString()));
    db.close();
#endif
}

void Test_Data_engine::test_form_creation() {
#if !DISABLE_ALL || 0
    std::stringstream input{R"(
		{
			"testA":{
				"data":[
					{	"name": "idA1",	 	"value": 100,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idA2",	 	"value": 200,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	}
				]
			},
			"testB":{
				"data":[
					{	"name": "idB1",	 	"value": 300,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idB2",	 	"value": 400,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idB3",	 	"value": 500,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	},
					{	"name": "idB4",	 	"value": 600,	"tolerance": 10,	"nice_name": "Betriebsspannung +5V"	}
				]
			}
		}
	)"};

    QMap<QString, QList<QVariant>> tags;
    Data_engine de{input, tags};
    const QString report_title = "Report Title";
    const QString image_footer_path = "";
    const QString image_header_path = "";
    QList<PrintOrderItem> print_order;
    de.generate_template("data engine test form autogenerated.lrxml", "db_name.db", report_title, image_footer_path, image_header_path, "", "", "", "", "", "",
                         print_order);
#endif
}

void Test_Data_engine::test_actual_value_statistic_get_latest_file_name() {
#if !DISABLE_ALL || 0
    QStringList sl{"C:/8061/33374719363430390605D61/report-2018_07_06-20_01_02-001.json", "C:/8061/33374719363430390605D61/report-2018_07_06-23_01_02-001.json",
                   "C:/8061/33374719363430390605D61/report-2018_07_06-19_54_22-001.json"};
    QDateTime dt = decode_date_time_from_file_name(sl[0].toStdString(), "report");
    QCOMPARE(dt.toString("yyyy_MM_dd-HH_mm_ss"), QString("2018_07_06-20_01_02"));
    QString result = select_newest_file_name(sl, "report");
    QCOMPARE(result, QString("C:/8061/33374719363430390605D61/report-2018_07_06-23_01_02-001.json"));

#endif
}
