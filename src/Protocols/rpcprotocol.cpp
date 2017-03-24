#include "rpcprotocol.h"
#include "channel_codec_wrapper.h"
#include "config.h"
#include "console.h"
#include "qt_util.h"
#include "rpc_ui.h"
#include "rpcruntime_decoded_function_call.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoded_function_call.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protocol_description.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTreeWidgetItem>
#include <cassert>
#include <fstream>

using namespace std::chrono_literals;

static void set_description_data(Device_data &dd, const RPCRuntimeDecodedParam &param, const std::string parent_field_name) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            std::string parameter_name = param.get_desciption()->get_parameter_name();
            const auto &param_value = param.as_string();

            if (parameter_name == "name") {
                dd.name = dd.name + QString().fromStdString(param_value);
            } else if (parameter_name == "version") {
                dd.version = dd.version + QString::fromStdString(param_value);
            } else if (parameter_name == "guid") {
                const auto &param_guid = param.as_full_string();
                dd.guid_bin = QByteArray::fromStdString(param_guid);
            } else {
                std::string field_name = param.get_desciption()->get_parameter_name();
                for (int i = 0; i < param.get_desciption()->as_array().number_of_elements; i++) {
                    set_description_data(dd, param.as_array()[i], field_name);
                }
            }
        }

        break;
        case RPCRuntimeParameterDescription::Type::character: {

        } break;
        case RPCRuntimeParameterDescription::Type::enumeration:
            break;
        case RPCRuntimeParameterDescription::Type::integer: {
            std::string parameter_name = param.get_desciption()->get_parameter_name();
            if (parameter_name == "") {
                parameter_name = parent_field_name;
            }
            if (parameter_name == "githash") {
                const auto &param_value = param.as_integer();
                dd.githash = QString::number(param_value, 16);
            } else if (parameter_name == "gitDate_unix") {
                const auto &param_value = param.as_integer();
                dd.gitDate_unix = QDateTime::fromTime_t(param_value).toString();
            } else if (parameter_name == "serialnumber") {
                const auto &param_value = param.as_integer();
                dd.serialnumber = QString::number(param_value);
            } else if (parameter_name == "deviceID") {
                const auto &param_value = param.as_integer();
                dd.deviceID = QString::number(param_value);
            } else if (parameter_name == "boardRevision") {
                const auto &param_value = param.as_integer();
                dd.boardRevision = QString::number(param_value);
            }
        } break;
        case RPCRuntimeParameterDescription::Type::structure: {
            std::string field_name = param.get_desciption()->get_parameter_name();
            for (auto &member : param.as_struct()) {
                set_description_data(dd, member.type, field_name);
            }
        } break;
    }
}

static Device_data get_description_data(const RPCRuntimeDecodedFunctionCall &call) {
    Device_data dd;
    for (auto &param : call.get_decoded_parameters()) {
        set_description_data(dd, param, "");
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
    return {{"GitHash", githash},   {"GitDate", gitDate_unix},     {"Serialnumber", serialnumber},
            {"DeviceID", deviceID}, {"GUID", guid_bin.toHex()},    {"BoardRevision", boardRevision},
            {"Name", name},         {"Serialnumber", serialnumber}};
}

RPCProtocol::RPCProtocol(CommunicationDevice &device, DeviceProtocolSetting &setting)
    : Protocol{"RPC"}
    , decoder{description}
    , encoder{description}
    , channel_codec{decoder}
    , device(&device)
    , device_protocol_setting(setting) {
    connection = QObject::connect(&device, &CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
        //qDebug() << "RPC-Protocol received" << data.size() << "bytes from device";
        cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
    });
    assert(connection);
}

RPCProtocol::~RPCProtocol() {
    assert(connection);
    auto result = QObject::disconnect(connection);
    assert(result);
}

bool RPCProtocol::is_correct_protocol() {
    // const CommunicationDevice::Duration TIMEOUT = std::chrono::milliseconds{100};
    int retries_per_transmission_backup = retries_per_transmission;
    retries_per_transmission = 0;
    auto result = call_and_wait(encoder.encode(0), device_protocol_setting.timeout);
    retries_per_transmission = retries_per_transmission_backup;
    if (result) {
        const auto &hash = QByteArray::fromStdString(result->get_parameter_by_name("hash_out")->as_full_string()).toHex();
        device->message(QObject::tr("Received Hash: ").toUtf8() + hash);
        QString folder = QSettings{}.value(Globals::rpc_xml_files_path_settings_key, QDir::currentPath()).toString();
        QString filename = hash + ".xml";
        QDirIterator directory_iterator(folder, QStringList{} << filename, QDir::Files, QDirIterator::Subdirectories);
        if (directory_iterator.hasNext() == false) {
            device->message(
                QObject::tr(
                    R"(Failed finding RPC description file "%1" in folder "%2" or any of its subfolders. Make sure it exists or change the search path in the settings menu.)")
                    .arg(folder, filename)
                    .toUtf8());
            return false;
        }
        auto filepath = directory_iterator.next();
        std::ifstream xmlfile(filepath.toStdString());
        if (description.openProtocolDescription(xmlfile) == false) {
            device->message(QObject::tr(R"(Failed opening RPC description file "%1".)").arg(filename).toUtf8());
            return false;
        }
        if (description.has_function("get_device_descriptor")) {
            auto get_device_descriptor_function = RPCRuntimeEncodedFunctionCall{description.get_function("get_device_descriptor")};
            if (get_device_descriptor_function.are_all_values_set()) {
                descriptor_answer = call_and_wait(get_device_descriptor_function, device_protocol_setting.timeout);
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

    return result != nullptr;
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call) {
    return call_and_wait(call, device_protocol_setting.timeout);
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice::Duration duration) {
    for (int try_count = 0; try_count <= retries_per_transmission; try_count++) {
        if (try_count != 0) {
            Console::debug()
                << QString(R"(Request for function "%1" timed out, retry %2)").arg(call.get_description()->get_function_name().c_str()).arg(try_count);
        }
        Utility::thread_call(device,
                             [ device = this->device, data = channel_codec.encode(call), display_data = call.encode() ] { device->send(data, display_data); });
        auto start = std::chrono::high_resolution_clock::now();
        auto check_received = [this, &start, &duration, &call]() -> std::unique_ptr<RPCRuntimeDecodedFunctionCall> {
            device->waitReceived(duration - (std::chrono::high_resolution_clock::now() - start), 1);
            if (channel_codec.transfer_complete()) { //found a reply
                auto transfer = channel_codec.pop_completed_transfer();
                auto &raw_data = transfer.get_raw_data();
                emit device->decoded_received(QByteArray(reinterpret_cast<const char *>(raw_data.data()), raw_data.size()));
                auto decoded_call = transfer.decode();
                if (decoded_call.get_id() == call.get_description()->get_reply_id()) { //found correct reply
                    return std::make_unique<RPCRuntimeDecodedFunctionCall>(std::move(decoded_call));
                } else { //found reply to something else, just gonna quietly ignore it
                    Console::debug()
                        << QString(R"(RPCProtocol::call_and_wait wrong answer. Expected answer to function "%1" but got answer to function "%2" instead.)")
                               .arg(call.get_description()->get_function_name().c_str(), decoded_call.get_declaration()->get_function_name().c_str());
                }
            }
            return nullptr;
        };
        do {
            auto retval = check_received();
            if (retval) {
                return retval;
            }
        } while (std::chrono::high_resolution_clock::now() - start < duration);
    }
    return nullptr;
}

const RPCRunTimeProtocolDescription &RPCProtocol::get_description() {
    return description;
}

QString RPCProtocol::get_device_summary() {
    return device_data.get_summary();
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

void RPCProtocol::get_lua_device_descriptor(sol::table &t) const {
    return device_data.get_lua_data(t);
}

RPCRuntimeEncodedFunctionCall RPCProtocol::encode_function(const std::string &name) const {
    return encoder.encode(name);
}

const channel_codec_instance_t *RPCProtocol::debug_get_channel_codec_instance() const {
    return channel_codec.debug_get_instance();
}

void RPCProtocol::clear() {
    while (channel_codec.transfer_complete()) {
        channel_codec.pop_completed_transfer();
    }
    channel_codec.reset_current_transfer();
}

std::string RPCProtocol::get_name() {
    return device_data.name.toStdString();
}
