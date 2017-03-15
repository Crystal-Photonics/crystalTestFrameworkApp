#include "test_data_engine.h"
#include "data_engine/data_engine.h"

#include <sstream>

using namespace std::string_literals;

void Test_Data_engine::basic_load_from_config() {
	std::stringstream input{R"({"":[]})"};
	Data_engine de{input};
}

void Test_Data_engine::check_properties_of_empty_set() {
	std::stringstream input{R"({"":[]})"};
	const Data_engine de{input};
	QVERIFY(de.is_complete());
	QVERIFY(de.all_values_in_range());
}

void Test_Data_engine::single_numeric_property_test() {
	std::stringstream input{R"({"":[{
							"name": "voltage",
							"value": 1000,
							"deviation": 100
							}]})"};
	Data_engine de{input};
	QVERIFY(!de.is_complete());
	QVERIFY(!de.all_values_in_range());
	QVERIFY(!de.value_in_range("voltage"));
	QCOMPARE(de.get_desired_value("voltage"), 1000.);
	QCOMPARE(de.get_desired_absolute_tolerance("voltage"), 100.);
	QCOMPARE(de.get_desired_relative_tolerance("voltage"), .1);
	QCOMPARE(de.get_desired_minimum("voltage"), 900.);
	QCOMPARE(de.get_desired_maximum("voltage"), 1100.);
	QCOMPARE(de.get_unit("voltage"), ""s);
	de.set_actual_number("voltage", 1000.1234);
	QVERIFY(de.is_complete());
	QVERIFY(de.all_values_in_range());
	QVERIFY(de.value_in_range("voltage"));
}

void Test_Data_engine::multiple_numeric_properties_test() {
	std::stringstream input{R"({"":[
							{
							"name": "voltage",
							"value": 1000,
							"deviation": 100
							},
							{
							"name": "current",
							"value": 200,
							"deviation": 100
							}
							]})"};
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
}
