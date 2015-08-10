#include "testcommunicationdevice.h"
#include "communicationdevice.h"
#include "socketcommunicationdevice.h"
#include "echocommunicationdevice.h"
#include <memory>

void TestCommunicationDevice::initTestCase(){

}

void TestCommunicationDevice::cleanupTestCase(){

}

void TestCommunicationDevice::test_parser_selection(){
	//socket
	{
		//positive checks
		for (auto &target : {
			 "client:127.0.0.1:1194",
			 "server:127.0.0.1:1194",
			 "client:0.0.0.0:0",
			 "server:0.0.0.0:0",
			 "client:255.255.255.255:65535",
			 "server:255.255.255.255:65535",
		}){
			try{
				std::unique_ptr<CommunicationDevice> c(CommunicationDevice::createConnection(target));
				QVERIFY2(c != nullptr, target);
				QVERIFY2(dynamic_cast<SocketCommunicationDevice *>(c.get()) != nullptr, target);
			}
			catch (const std::runtime_error &e){ //failing to open a port is not a test failure
				auto typeIpPort = QString(target).split(':');
				QCOMPARE(QString::fromStdString(e.what()), "Failed opening " + typeIpPort[1] + ':' + typeIpPort[2]);
			}
		}
		//negative checks
		for (auto &target : {
			 "127.0.0.1:1194",
			 "0.0.0.0:0",
			 "255.255.255.255:65535",
			 "client:0.0.1220.0:0",
			 "server:0.0.1220.0:0",
			 "client:1127.0.0.1:1194",
			 "server:1127.0.0.1:1194",
			 "client:0.0.0.1234:0",
			 "server:0.0.0.1234:0",
			 "client:255.255.255.255:655350",
			 "server:255.255.255.255:655350",
		}){
			std::unique_ptr<CommunicationDevice> c(CommunicationDevice::createConnection(target));
			QVERIFY2(c == nullptr, target);
		}
	}
	//echo
	{
		std::unique_ptr<CommunicationDevice> c(CommunicationDevice::createConnection("echo"));
		QVERIFY2(c != nullptr, "Failed creating echo device");
		QVERIFY2(dynamic_cast<EchoCommunicationDevice *>(c.get()) != nullptr, "Created wrong device for target \"echo\"");
	}
}
