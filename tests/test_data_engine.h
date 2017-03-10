#ifndef TEST_DATA_ENGINE_H
#define TEST_DATA_ENGINE_H

#include "autotest.h"
#include <QObject>

class Test_Data_engine : public QObject {
	Q_OBJECT
	public:
	private slots:
	void load_from_config();
};

DECLARE_TEST(Test_Data_engine)

#endif // TEST_DATA_ENGINE_H
