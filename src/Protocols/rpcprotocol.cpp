#include "rpcprotocol.h"
#include "console.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"

#include <QObject>
#include <cassert>

using namespace std::chrono_literals;

bool RPCProtocol::is_correct_protocol(CommunicationDevice &device) {
	//TODO: Send a 0, check if we get a hash back
	RPCRunTimeProtocolDescription rpcinterpreter;
	RPCRuntimeEncoder encoder(rpcinterpreter);
	auto request_function = encoder.encode(0);
	auto data = request_function.encode();

	device.send(QByteArray(reinterpret_cast<const char *>(data.data()), data.size()));
	QByteArray received_data;
	auto connection = QObject::connect(&device, &CommunicationDevice::received, [&received_data](const QByteArray &data) { received_data = data; });
	assert(connection);
	device.waitReceived(1000ms);
	QObject::disconnect(connection);
	Console::note() << QObject::tr("received %1 bytes of data from probing %2").arg(received_data.size()).arg(device.getTarget());
	return received_data.size() > 0; //TODO: check if we have the correct amount of data and look up the hash
}
