#ifndef TESTCOMMUNICATIONDEVICE_H
#define TESTCOMMUNICATIONDEVICE_H

#include <QObject>
#include "autotest.h"

class TestCommunicationDevice : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();
	void test_parser_selection();

};
DECLARE_TEST(TestCommunicationDevice)

#endif // TESTCOMMUNICATIONDEVICE_H
