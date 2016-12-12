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
#include <QDateTime>
#include <QDir>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTreeWidgetItem>
#include <cassert>
#include <fstream>

using namespace std::chrono_literals;

static void set_description_data(Device_data &dd, const RPCRuntimeDecodedParam &param) {
	switch (param.get_desciption()->get_type()) {
		case RPCRuntimeParameterDescription::Type::array:
			for (int i = 0; i < param.get_desciption()->as_array().number_of_elements; i++) {
				set_description_data(dd, param.as_array()[i]);
			}
			break;
		case RPCRuntimeParameterDescription::Type::character:
			break;
		case RPCRuntimeParameterDescription::Type::enumeration:
			break;
		case RPCRuntimeParameterDescription::Type::integer:
			if (param.get_desciption()->get_parameter_name() == "githash") {
				const auto &param_value = param.as_integer();
				dd.githash = QString::number(param_value, 16);
			} else if (param.get_desciption()->get_parameter_name() == "gitDate_unix") {
				const auto &param_value = param.as_integer();
				dd.gitDate_unix = QDateTime::fromTime_t(param_value).toString();
			} else if (param.get_desciption()->get_parameter_name() == "serialnumber") {
				const auto &param_value = param.as_integer();
				dd.serialnumber = QString::number(param_value);
			} else if (param.get_desciption()->get_parameter_name() == "deviceID") {
				const auto &param_value = param.as_integer();
				dd.deviceID = QString::number(param_value);
			} else if (param.get_desciption()->get_parameter_name() == "boardRevision") {
				const auto &param_value = param.as_integer();
				dd.boardRevision = QString::number(param_value);
			}
			break;
		case RPCRuntimeParameterDescription::Type::structure:
			for (auto &member : param.as_struct()) {
				set_description_data(dd, member.type);
			}
			break;
	}
}

static Device_data get_description_data(const RPCRuntimeDecodedFunctionCall &call) {
	Device_data dd;
	for (auto &param : call.get_decoded_parameters()) {
		set_description_data(dd, param);
	}
	return dd;
}

static void set_display_data(QTreeWidgetItem *item, const Device_data &data) {
	const auto &summary = data.get_summary();
	item->setToolTip(0, summary);
	item->addChild(new QTreeWidgetItem(item, QStringList{} << summary));
}

QString Device_data::get_summary() const {
	QStringList statustip;
	for (auto &d : get_description_source()) {
		if (d.source.isEmpty() == false) {
			statustip << d.description + ": " + d.source;
		}
	}
	return statustip.join("\n");
}

void Device_data::get_lua_data(sol::table &t) const {
	for (auto &d : get_description_source()) {
		t.set(d.description.toStdString(), d.source.toStdString());
	}
}

std::vector<Device_data::Description_source> Device_data::get_description_source() const {
	return {{"GitHash", githash},
			{"GitDate", gitDate_unix},
			{"Serialnumber", serialnumber},
			{"DeviceID", deviceID},
			{"GUID", guid},
			{"BoardRevision", boardRevision},
			{"Name", name},
			{"Serialnumber", serialnumber}};
}

RPCProtocol::RPCProtocol(CommunicationDevice &device)
	: Protocol{"RPC"}
	, decoder{description}
	, encoder{description}
	, channel_codec{decoder}
	, device(&device) {
	connection = QObject::connect(&device, &CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
		cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
	});
	assert(connection);
}

RPCProtocol::~RPCProtocol() {
	assert(connection);
	auto result = QObject::disconnect(connection);
	assert(result);
}

RPCProtocol::RPCProtocol(RPCProtocol &&other)
	: Protocol{"RPC"}
	, description(std::move(other.description))
	, decoder(std::move(other.decoder))
	, encoder(std::move(other.encoder))
	, channel_codec(std::move(other.channel_codec))
	, descriptor_answer(std::move(other.descriptor_answer))
	, device(other.device)
	, device_data(std::move(other.device_data)) {
	connection = QObject::connect(device, &CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
		cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
	});
	assert(connection);
}

bool RPCProtocol::is_correct_protocol() {
	auto result = call_and_wait(encoder.encode(0));
	if (result) {
		const auto &hash = QByteArray::fromStdString(result->get_parameter_by_name("hash_out")->as_string()).toHex();
		device->message(QObject::tr("Received Hash: ").toUtf8() + hash);
		const auto filename =
			QDir(QSettings{}.value(Globals::rpc_xml_files_path_settings_key, QDir::currentPath()).toString()).filePath(hash + ".xml").toStdString();
		std::ifstream xmlfile(filename);
		if (description.openProtocolDescription(xmlfile) == false) {
			device->message(QObject::tr("Failed opening RPC description file %1. Make sure it exists or change the search path in the settings menu.")
								.arg(filename.c_str())
								.toUtf8());
		} else {
			if (description.has_function("get_device_descriptor")) {
				auto get_device_descriptor_function = RPCRuntimeEncodedFunctionCall{description.get_function("get_device_descriptor")};
				if (get_device_descriptor_function.are_all_values_set()) {
					descriptor_answer = call_and_wait(get_device_descriptor_function);
					if (descriptor_answer) {
						device_data = get_description_data(*descriptor_answer);
					}
				} else {
					Console::note() << "RPC-function \"get_device_descriptor\" requires unknown parameters";
				}
			} else {
				Console::note() << "No RPC-function \"get_device_descriptor\" available";
			}
		}
	}
	return result != nullptr;
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice::Duration duration) {
	const auto data = channel_codec.encode(call);
	device->send(data, call.encode());
	auto start = std::chrono::high_resolution_clock::now();
	while (std::chrono::high_resolution_clock::now() - start < duration) {
		if (channel_codec.transfer_complete()) { //found a reply
			auto transfer = channel_codec.pop_completed_transfer();
			auto &raw_data = transfer.get_raw_data();
			emit device->decoded_received(QByteArray(reinterpret_cast<const char *>(raw_data.data()), raw_data.size()));
			auto decoded_call = transfer.decode();
			if (decoded_call.get_id() == call.get_description()->get_reply_id()) { //found correct reply
				return std::make_unique<RPCRuntimeDecodedFunctionCall>(std::move(decoded_call));
			} else { //found reply to something else, just gonna quietly ignore it
			}
		}
		device->waitReceived(duration - (std::chrono::high_resolution_clock::now() - start));
	}
	return nullptr;
}

const RPCRunTimeProtocolDescription &RPCProtocol::get_description() {
	return description;
}

void RPCProtocol::set_ui_description(QTreeWidgetItem *ui_entry) {
	if (descriptor_answer) {
		auto data = get_description_data(*descriptor_answer);
		ui_entry->setText(1, "RPC");
		ui_entry->setText(2, data.name);
		set_display_data(ui_entry, data);
	} else {
		Console::note() << "RPC-function \"get_device_descriptor\" did not answer";
		//TODO: add a rightclick action to resend the descriptor request
		//ui_entry->
	}
}

void RPCProtocol::get_lua_device_descriptor(sol::table &t) const
{
	return device_data.get_lua_data(t);
}
