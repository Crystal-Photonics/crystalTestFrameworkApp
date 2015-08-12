#include "testsocketcommunicationdevice.h"
#include "communicationdevice.h"
#include <QByteArray>
#include "util.h"
#include <QSignalSpy>

void TestSocketCommunicationDevice::initTestCase(){

}

void TestSocketCommunicationDevice::cleanupTestCase(){

}

void TestSocketCommunicationDevice::selfConnect(){
	auto server = CommunicationDevice::createConnection("server:127.0.0.1:1192");
	auto client = CommunicationDevice::createConnection("client:127.0.0.1:1192");
	QByteArray serverReceivedData;
	QByteArray clientReceivedData;
	auto serverReceive = [&serverReceivedData](QByteArray data){
		serverReceivedData = std::move(data);
	};
	auto clientReceive = [&clientReceivedData](QByteArray data){
		clientReceivedData = std::move(data);
	};
	auto serverSignalSlotConnection = connect(server.get(), CommunicationDevice::receive, serverReceive);
	QVERIFY2(serverSignalSlotConnection, "Failed signal slot connection on server");
	auto closeServerSocketConnection = Utility::RAII_do(
		[&serverSignalSlotConnection]()
		{
			QObject::disconnect(serverSignalSlotConnection);
		}
	);
	auto clientSignalSlotConnection = connect(client.get(), CommunicationDevice::receive, clientReceive);
	QVERIFY2(clientSignalSlotConnection, "Failed signal slot connection on client");
	auto closeClientSocketConnection = Utility::RAII_do(
		[&clientSignalSlotConnection]()
		{
			QObject::disconnect(clientSignalSlotConnection);
		}
	);
	//QApplication::processEvents(QEventLoop::AllEvents, 500);
	bool success;
	success = client->waitConnected();
	assert(success);
	success = server->waitConnected();
	assert(success);
	QVERIFY2(client->isConnected(), "Client failed to connect");
	QVERIFY2(server->isConnected(), "Server failed to connect");
	auto clientSend = QByteArray("Hello Server!");
	auto serverSend = QByteArray("Hello Client!");
	server->send(serverSend);
	client->send(clientSend);
	if (serverReceivedData.size() == 0) //avoid timeout when data already arrived
		server->waitReceive();
	if (clientReceivedData.size() == 0) //avoid timeout when data already arrived
		client->waitReceive();
	QCOMPARE(serverReceivedData, clientSend);
	QCOMPARE(clientReceivedData, serverSend);
}
