#include "rpcprotocol.h"
#include "channel_codec_wrapper.h"
#include "config.h"
#include "console.h"
#include "rpc_ui.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"

#include <QByteArray>
#include <QDir>
#include <QObject>
#include <QSettings>
#include <QTreeWidgetItem>
#include <cassert>
#include <fstream>

using namespace std::chrono_literals;

RPCProtocol::RPCProtocol()
	: decoder{description}
	, encoder{description}
	, channel_codec{decoder} {}

RPCProtocol::~RPCProtocol()
{
	QObject::disconnect(connection);
}

bool RPCProtocol::is_correct_protocol(CommunicationDevice &device) {
	connection = QObject::connect(&device, &CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
		cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
	});
	auto result = call_and_wait(encoder.encode(0), device);
	if (result) {
		const auto &hash = QByteArray::fromStdString(result->get_parameter_by_name("hash_out")->as_string()).toHex();
		device.message(QObject::tr("Received Hash: ").toUtf8() + hash);
		const auto filename =
			QDir(QSettings{}.value(Globals::rpc_xml_files_path_settings_key, QDir::currentPath()).toString()).filePath(hash + ".xml").toStdString();
		std::ifstream xmlfile(filename);
		if (description.openProtocolDescription(xmlfile) == false) {
			device.message(QObject::tr("Failed opening RPC description file %1. Make sure it exists or change the search path in the settings menu.")
							   .arg(filename.c_str())
							   .toUtf8());
		}
	}
	return result != nullptr;
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

const RPCRunTimeProtocolDescription &RPCProtocol::get_description() {
	return description;
}

void RPCProtocol::set_ui_description(CommunicationDevice &device, QTreeWidgetItem *ui_entry) {
	const auto &protocol_description = get_description();
	if (protocol_description.has_function("get_device_descriptor")) {
		auto get_device_descriptor_function = RPCRuntimeEncodedFunctionCall{protocol_description.get_function("get_device_descriptor")};
		if (get_device_descriptor_function.are_all_values_set()) {
			auto result = call_and_wait(get_device_descriptor_function, device);
			if (result) {
				ui_entry->addChild(getTreeWidgetReport(*result).release());
			}
			else{
				Console::note() << "RPC-function \"get_device_descriptor\" did not answer";
			}
		}
		else{
			Console::note() << "RPC-function \"get_device_descriptor\" requires parameters";
		}
	}
	else{
		Console::note() << "No RPC-function \"get_device_descriptor\"";
	}
}
