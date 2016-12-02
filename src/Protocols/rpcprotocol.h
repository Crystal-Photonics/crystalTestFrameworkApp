#ifndef RPCPROTOCOL_H
#define RPCPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"
#include "channel_codec_wrapper.h"
#include "protocol.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"
#include "rpcruntime_transfer.h"

#include <memory>

class RPCRuntimeEncodedFunctionCall;
class QTreeWidgetItem;

class RPCProtocol : public Protocol {
	public:
	RPCProtocol();
	~RPCProtocol();
	RPCProtocol(const RPCProtocol &) = delete;
	RPCProtocol(RPCProtocol &&other);
	bool is_correct_protocol(CommunicationDevice &device);
	std::unique_ptr<RPCRuntimeDecodedFunctionCall> call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice &device,
																 CommunicationDevice::Duration duration = std::chrono::seconds{1});
	const RPCRunTimeProtocolDescription &get_description();
	void set_ui_description(CommunicationDevice &device, QTreeWidgetItem *ui_entry);

	private:
	RPCRunTimeProtocolDescription description;
	RPCRuntimeDecoder decoder;
	RPCRuntimeEncoder encoder;
	Channel_codec_wrapper channel_codec;
	QMetaObject::Connection connection;
	std::unique_ptr<RPCRuntimeDecodedFunctionCall> descriptor_answer;
};

#endif // RPCPROTOCOL_H
