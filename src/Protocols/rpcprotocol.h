#ifndef RPCPROTOCOL_H
#define RPCPROTOCOL_H

#include "CommunicationDevices/communicationdevice.h"
#include "channel_codec_wrapper.h"
#include "device_protocols_settings.h"
#include "protocol.h"
#include "rpcruntime_decoder.h"
#include "rpcruntime_encoder.h"
#include "rpcruntime_protcol.h"
#include "rpcruntime_protocol_description.h"
#include <memory>
#include <sol_forward.hpp>

class QTreeWidgetItem;
class RPCRuntimeEncodedFunctionCall;

struct Device_data {
    QString githash;
    QString gitDate_unix;

    QString serialnumber;
    QString deviceID;
    QByteArray guid_bin;
    QString boardRevision;

    QString name;
    QString version;

    QString get_summary() const;
    void get_lua_data(sol::table &t) const;

    private:
    struct Description_source {
        QString description;
        const QString source;
    };
    std::vector<Description_source> get_description_source() const;
};

class CommunicationDeviceWrapper : public RPCIODevice {
    public:
    CommunicationDeviceWrapper(CommunicationDevice &device);

    void send(std::vector<unsigned char> data, std::vector<unsigned char> pre_encodec_data) override;
    bool waitReceived(std::chrono::steady_clock::duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling = false) override;

    private:
    CommunicationDevice &com_device;
};

struct RPCTimeoutException : std::runtime_error {
    RPCTimeoutException(std::string message)
        : std::runtime_error{std::move(message)} {}
};

class RPCProtocol : public Protocol {
    public:
    RPCProtocol(CommunicationDevice &device, DeviceProtocolSetting setting);
    ~RPCProtocol();
    RPCProtocol(const RPCProtocol &) = delete;
    RPCProtocol(RPCProtocol &&other) = delete;
    bool is_correct_protocol();
    std::unique_ptr<RPCRuntimeDecodedFunctionCall> call_and_wait(const RPCRuntimeEncodedFunctionCall &call, bool show_messagebox_when_timeout);
    std::unique_ptr<RPCRuntimeDecodedFunctionCall> call_and_wait(const RPCRuntimeEncodedFunctionCall &call, CommunicationDevice::Duration duration,
                                                                 bool show_messagebox_when_timeout);
    const RPCRunTimeProtocolDescription &get_description();
    void set_ui_description(QTreeWidgetItem *ui_entry);
    RPCProtocol &operator=(const RPCProtocol &&) = delete;
    void get_lua_device_descriptor(sol::table &t) const;
    RPCRuntimeEncodedFunctionCall encode_function(const int request_id) const;
    RPCRuntimeEncodedFunctionCall encode_function(const std::string &name) const;
    bool has_function(const std::string &name) const;
    RPCFunctionCallResult call_get_hash_function() const;
    RPCFunctionCallResult call_get_hash_function(int retries) const;
    void clear();
    std::string get_name();
    QString get_device_summary();
    QString get_manual() const override;

    private:
    std::unique_ptr<RPCRuntimeProtocol> rpc_runtime_protocol;

    void console_message(RPCConsoleLevel level, QString message);

    QMetaObject::Connection console_message_connection;
    std::unique_ptr<RPCRuntimeDecodedFunctionCall> descriptor_answer;
    Device_data device_data;
    CommunicationDeviceWrapper communication_wrapper;
    DeviceProtocolSetting device_protocol_setting;
};

#endif // RPCPROTOCOL_H
