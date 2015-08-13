#include "testechocommunicationdevice.h"
#include "communicationdevice.h"
#include "echocommunicationdevice.h"

#include <memory>
#include <QByteArray>

void TestEchoCommunicationDevice::initTestCase()
{

}

void TestEchoCommunicationDevice::cleanupTestCase()
{

}

void TestEchoCommunicationDevice::signaltest()
{
	std::unique_ptr<CommunicationDevice> c(CommunicationDevice::createConnection("echo"));
	QVERIFY2(c != nullptr, "Failed creating echo device");
	QVERIFY2(dynamic_cast<EchoCommunicationDevice *>(c.get()) != nullptr, "Created wrong device for target \"echo\"");
	QByteArray data;
	auto receiver = [&data](QByteArray received){
		data = std::move(received);
	};
	auto connection = QObject::connect(dynamic_cast<EchoCommunicationDevice *>(c.get()), &EchoCommunicationDevice::receive, receiver);
	QVERIFY2(connection, "Failed connecting communication device to lambda");
	const QByteArray testData = "RandomText";
	//test receive directly
	emit c->receive(testData);
	QCOMPARE(data, QByteArray(testData));
	data.clear();
	QCOMPARE(data, QByteArray());
	//test if send redirects to receive
	c->send(testData);
	QCOMPARE(data, QByteArray(testData));
}
