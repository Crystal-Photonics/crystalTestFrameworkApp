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
			 "127.0.0.1:1194",
			 "0.0.0.0:0",
			 "255.255.255.255:65535",
		}){
			std::unique_ptr<CommunicationDevice> c(CommunicationDevice::createConnection(target));
			QVERIFY2(c != nullptr, target);
			QVERIFY2(dynamic_cast<SocketCommunicationDevice *>(c.get()) != nullptr, target);
		}
		//negative checks
		for (auto &target : {
			 "1127.0.0.1:1194",
			 "0.0.1220.0:0",
			 "0.0.0.1234:0",
			 "255.255.255.255:655350",
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
