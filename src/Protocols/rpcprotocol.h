#ifndef RPCPROTOCOL_H
#define RPCPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"
#include "channel_codec_wrapper.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"
#include "rpcruntime_transfer.h"

#include <memory>

class RPCRuntimeEncodedFunctionCall;

class RPCProtocol {
	public:
	RPCProtocol();
	bool is_correct_protocol(CommunicationDevice &device);
	std::unique_ptr<RPCRuntimeDecodedFunctionCall> call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice &device,
																 CommunicationDevice::Duration duration = std::chrono::seconds{1});

	private:
	RPCRunTimeProtocolDescription interpreter;
	RPCRuntimeDecoder decoder;
	RPCRuntimeEncoder encoder;
	Channel_codec_wrapper channel_codec;
};

#endif // RPCPROTOCOL_H
