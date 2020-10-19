#include "rpcprotocol.h"
#include "Windows/mainwindow.h"
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
#include <sol.hpp>

using namespace std::chrono_literals;

static void set_description_data(Device_data &dd, const RPCRuntimeDecodedParam &param, const std::string parent_field_name) {
    switch (param.get_desciption()->get_type()) {
        case RPCRuntimeParameterDescription::Type::array: {
            std::string parameter_name = param.get_desciption()->get_parameter_name();
            const auto &param_value = param.as_string();

            if (parameter_name == "name") {
                dd.name = dd.name + QString::fromStdString(param_value);
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
    return {{"GitHash", githash},   {"GitDate", gitDate_unix},  {"Serialnumber", serialnumber},
            {"DeviceID", deviceID}, {"GUID", guid_bin.toHex()}, {"BoardRevision", boardRevision},
            {"Name", name},         {"Version", version}};
}

RPCProtocol::RPCProtocol(CommunicationDevice &device, DeviceProtocolSetting setting)
    : Protocol{"RPC"}
    , communication_wrapper(device)
    , device_protocol_setting(std::move(setting)) {
    rpc_runtime_protocol = std::make_unique<RPCRuntimeProtocol>(communication_wrapper, device_protocol_setting.timeout);
#if 1
    console_message_connection =
        QObject::connect(rpc_runtime_protocol.get(), &RPCRuntimeProtocol::console_message, [this](RPCConsoleLevel level, const QString &data) { //
            this->console_message(level, data);
        });
    assert(console_message_connection);
#endif

#if 0
    connection = QObject::connect(&device, &CommunicationDevice::received, [&cc = channel_codec](const QByteArray &data) {
        //qDebug() << "RPC-Protocol received" << data.size() << "bytes from device";
        cc.add_data(reinterpret_cast<const unsigned char *>(data.data()), data.size());
    });
#endif
    // QObject::connect(rpc_runtime_protocol.get(), SIGNAL(console_message(RPCConsoleLevel,QString)), this, SLOT());
}

RPCProtocol::~RPCProtocol() {
    assert(console_message_connection);
    auto result = QObject::disconnect(console_message_connection);
    assert(result);
}

bool RPCProtocol::is_correct_protocol() {
    // const CommunicationDevice::Duration TIMEOUT = std::chrono::milliseconds{100};
    if (rpc_runtime_protocol.get()->load_xml_file(QSettings{}.value(Globals::rpc_xml_files_path_settings_key, QDir::currentPath()).toString())) {
        if (rpc_runtime_protocol.get()->description.has_function("get_device_descriptor")) {
            auto get_device_descriptor_function = RPCRuntimeEncodedFunctionCall{rpc_runtime_protocol.get()->description.get_function("get_device_descriptor")};
            if (get_device_descriptor_function.are_all_values_set()) {
                descriptor_answer = call_and_wait(get_device_descriptor_function, device_protocol_setting.timeout, true);
                if (descriptor_answer) {
                    device_data = get_description_data(*descriptor_answer);
                }
            } else {
                Console_handle::note() << "RPC-function \"get_device_descriptor\" requires unknown parameters";
            }
        } else {
            Console_handle::note() << "No RPC-function \"get_device_descriptor\" available";
        }
        return true;
    }

    return false;
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call, bool show_messagebox_when_timeout) {
    return call_and_wait(call, device_protocol_setting.timeout, show_messagebox_when_timeout);
}

std::unique_ptr<RPCRuntimeDecodedFunctionCall> RPCProtocol::call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice::Duration duration,
                                                                          bool show_messagebox_when_timeout) {
    do {
        auto result = rpc_runtime_protocol.get()->call_and_wait(call, duration);
        if (result.error == RPCError::success) {
            return std::move(result.decoded_function_call_reply);
        }
    } while (Utility::promised_thread_call(MainWindow::mw, [this, function_name = call.get_description()->get_function_name(), show_messagebox_when_timeout] {
        if (show_messagebox_when_timeout) {
            return QMessageBox::warning(nullptr, QObject::tr("CrystalTestFramework - Timeout error"),
                                        QObject::tr("The device \"%1\" failed to respond to function \"%2\".\nDo you want to try again?")
                                            .arg(device_data.name)
                                            .arg(function_name.c_str()),
                                        QMessageBox::Retry | QMessageBox::Abort) == QMessageBox::Retry;
        } else {
            return false;
        }
    }));
    throw RPCTimeoutException{QObject::tr("Timeout in RPC function \"%1\".").arg(call.get_description()->get_function_name().c_str()).toStdString()};
}

const RPCRunTimeProtocolDescription &RPCProtocol::get_description() {
    return rpc_runtime_protocol.get()->description;
}

QString RPCProtocol::get_device_summary() {
    return device_data.get_summary();
}

QString RPCProtocol::get_manual() const {
    return QString::fromStdString(rpc_runtime_protocol->get_xml_file_path());
}

void RPCProtocol::console_message(RPCConsoleLevel level, QString message) {
    switch (level) {
        case RPCConsoleLevel::note:
            Console_handle::note() << message;
            break;

        case RPCConsoleLevel::error:
            Console_handle::error() << message;
            break;

        case RPCConsoleLevel::debug:
            Console_handle::debug() << message;
            break;

        case RPCConsoleLevel::warning:
            Console_handle::warning() << message;
            break;
    }
}

void RPCProtocol::set_ui_description(QTreeWidgetItem *ui_entry) {
    assert(QThread::currentThread() == MainWindow::gui_thread);
    if (descriptor_answer) {
        auto data = get_description_data(*descriptor_answer);
        ui_entry->setText(1, "RPC");
        ui_entry->setText(2, data.name);
        set_display_data(ui_entry, data);
    } else {
        Console_handle::note() << "RPC-function \"get_device_descriptor\" did not answer";
        //TODO: add a rightclick action to resend the descriptor request
        //ui_entry->
    }
}

void RPCProtocol::get_lua_device_descriptor(sol::table &t) const {
    assert(not currently_in_gui_thread());
    return device_data.get_lua_data(t);
}

RPCRuntimeEncodedFunctionCall RPCProtocol::encode_function(const std::string &name) const {
    return rpc_runtime_protocol->encode_function(name);
}

bool RPCProtocol::has_function(const std::string &name) const {
    return rpc_runtime_protocol->function_exists_for_encoding(name);
}

RPCFunctionCallResult RPCProtocol::call_get_hash_function() const {
    return rpc_runtime_protocol->call_get_hash_function();
}

RPCFunctionCallResult RPCProtocol::call_get_hash_function(int retries) const {
    return rpc_runtime_protocol->call_get_hash_function(retries);
}

RPCRuntimeEncodedFunctionCall RPCProtocol::encode_function(const int request_id) const {
    return rpc_runtime_protocol->encode_function(request_id);
}

void RPCProtocol::clear() {
    rpc_runtime_protocol->clear();
}

std::string RPCProtocol::get_name() {
    return device_data.name.toStdString();
}

CommunicationDeviceWrapper::CommunicationDeviceWrapper(CommunicationDevice &device)
    : com_device(device) {
    connect(this, &CommunicationDeviceWrapper::decoded_received, &com_device, &CommunicationDevice::decoded_received);
    connect(this, &CommunicationDeviceWrapper::message, &com_device, &CommunicationDevice::message);
    connect(&com_device, &CommunicationDevice::received, this, &CommunicationDeviceWrapper::received);
}

void CommunicationDeviceWrapper::send(std::vector<unsigned char> data, std::vector<unsigned char> pre_encodec_data) {
    com_device.send(data, pre_encodec_data);
}

bool CommunicationDeviceWrapper::waitReceived(std::chrono::_V2::steady_clock::duration timeout, int bytes, bool isPolling) {
    return com_device.waitReceived(timeout, bytes, isPolling);
}
