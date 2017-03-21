#include "test_data_engine.h"
#include "data_engine/data_engine.h"

#include <sstream>
#include <QString>

QString operator "" _qs(const char *text, std::size_t size){
	return QString::fromStdString(std::string{text, text + size});
}

void Test_Data_engine::basic_load_from_config() {
	std::stringstream input{R"([])"};
	Data_engine de{input};
}

void Test_Data_engine::check_properties_of_empty_set() {
	std::stringstream input{R"([])"};
	const Data_engine de{input};
	QVERIFY(de.is_complete());
	QVERIFY(de.all_values_in_range());
}

void Test_Data_engine::single_numeric_property_test() {
	std::stringstream input{R"([{
							"name": "voltage",
							"value": 1000,
							"deviation": 100
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
}

void Test_Data_engine::multiple_numeric_properties_test() {
	std::stringstream input{R"([
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
}

void Test_Data_engine::test_text_entry() {
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
}

void Test_Data_engine::test_preview() {
	std::stringstream input{R"([
							{
								"name": "voltage",
								"value": 1000,
								"deviation": 100
							},
							{
								"name": "current",
								"value": "[voltage]",
								"deviation": 100
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
	de.generate_pdf("test.pdf");
}
