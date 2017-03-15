#ifndef TEST_DATA_ENGINE_H
#define TEST_DATA_ENGINE_H

#include "autoTest.h"
#include <QObject>

class Test_Data_engine : public QObject {
	Q_OBJECT
	public:
	private slots:
	void basic_load_from_config();
	void check_properties_of_empty_set();
	void single_numeric_property_test();
	void multiple_numeric_properties_test();
	void test_text_entry();
};

DECLARE_TEST(Test_Data_engine)

#endif // TEST_DATA_ENGINE_H
