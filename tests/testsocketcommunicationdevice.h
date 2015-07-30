#ifndef TESTSOCKETCOMMUNICATIONDEVICE_H
#define TESTSOCKETCOMMUNICATIONDEVICE_H

#include "autotest.h"

class TestSocketCommunicationDevice : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();
	void selfConnect();
};

DECLARE_TEST(TestSocketCommunicationDevice)

#endif // TESTSOCKETCOMMUNICATIONDEVICE_H
