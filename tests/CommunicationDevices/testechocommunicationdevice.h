#ifndef TESTECHOCOMMUNICATIONDEVICE_H
#define TESTECHOCOMMUNICATIONDEVICE_H

#include <QObject>
#include "autotest.h"

class TestEchoCommunicationDevice : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();
	void signaltest();
};
DECLARE_TEST(TestEchoCommunicationDevice)
#endif // TESTECHOCOMMUNICATIONDEVICE_H
