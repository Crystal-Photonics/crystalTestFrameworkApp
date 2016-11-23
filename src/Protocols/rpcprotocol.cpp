#include "rpcprotocol.h"
#include "channel_codec_wrapper.h"
#include "console.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"

#include <QByteArray>
#include <QObject>
#include <cassert>

using namespace std::chrono_literals;

RPCProtocol::RPCProtocol()
	: decoder{interpreter}
	, encoder{interpreter}
	, channel_codec{decoder} {}

bool RPCProtocol::is_correct_protocol(CommunicationDevice &device) {
	auto connection = QObject::connect(&device, &CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
		cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
	});
	auto result = call_and_wait(encoder.encode(0), device);
	QObject::disconnect(connection);
	if (result) {
		const auto &hash = QByteArray::fromStdString(result->get_parameter_by_name("hash_out")->as_string()).toHex();
		device.message(QObject::tr("Received Hash: ").toUtf8() + hash);
	}
	return result != nullptr; //TODO: use result to get the hash and load the XML
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice &device,
																		  CommunicationDevice::Duration duration) {
	const auto data = channel_codec.encode(call);
	device.send(data, call.encode());
	auto start = std::chrono::high_resolution_clock::now();
	while (std::chrono::high_resolution_clock::now() - start < duration) {
		if (channel_codec.transfer_complete()) { //found a reply
			auto transfer = channel_codec.pop_completed_transfer();
			auto &raw_data = transfer.get_raw_data();
			emit device.decoded_received(QByteArray(reinterpret_cast<const char *>(raw_data.data()), raw_data.size()));
			auto decoded_call = transfer.decode();
			if (decoded_call.get_id() == call.get_description()->get_reply_id()) { //found correct reply
				return std::make_unique<RPCRuntimeDecodedFunctionCall>(std::move(decoded_call));
			} else { //found reply to something else, just gonna quietly ignore it
			}
		}
		device.waitReceived(duration - (std::chrono::high_resolution_clock::now() - start));
	}
	return nullptr;
}
