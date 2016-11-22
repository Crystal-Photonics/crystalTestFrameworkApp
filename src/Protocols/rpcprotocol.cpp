#include "rpcprotocol.h"
#include "channel_codec_wrapper.h"
#include "console.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"

#include <QObject>
#include <cassert>

using namespace std::chrono_literals;

RPCProtocol::RPCProtocol()
	: decoder{interpreter}
	, encoder{interpreter}
	, channel_codec{decoder} {}

bool RPCProtocol::is_correct_protocol(CommunicationDevice &device) {
	auto connection = QObject::connect(&device, CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
		cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
	});
	auto result = call_and_wait(encoder.encode(0), device);
	QObject::disconnect(connection);
	return result != nullptr; //TODO: use result to get the hash and load the XML
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice &device,
																		  CommunicationDevice::Duration duration) {
	const auto data = channel_codec.encode(call);
	device.send(QByteArray(reinterpret_cast<const char *>(data.data()), data.size()));
	auto start = std::chrono::high_resolution_clock::now();
	while (std::chrono::high_resolution_clock::now() - start < duration) {
		if (channel_codec.transfer_complete()) { //found a reply
			auto transfer = channel_codec.pop();
			if (transfer.get_id() == call.get_description()->get_reply_id()) { //found correct reply
				return std::make_unique<RPCRuntimeDecodedFunctionCall>(std::move(transfer));
			} else { //found reply to something else, just gonna quietly ignore it
			}
		}
		device.waitReceived(duration - (std::chrono::high_resolution_clock::now() - start));
	}
	return nullptr;
}
