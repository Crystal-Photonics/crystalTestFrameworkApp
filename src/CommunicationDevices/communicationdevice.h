#ifndef COMMUNICATIONDEVICE_H
#define COMMUNICATIONDEVICE_H

#include "export.h"

#include <QMap>
#include <QObject>
#include <QVariant>
#include <atomic>
#include <chrono>
#include <memory>

class QByteArray;
const QString HOST_NAME_TAG = "host-name";
const QString TYPE_NAME_TAG = "protocol-name";
const QString BAUD_RATE_TAG = "baudrate";
const QString DEVICE_MANUAL_NAME_TAG = "manual-name";
const QString DEVICE_MANUAL_SN_TAG = "sn";

class EXPORT CommunicationDevice : public QObject {
    Q_OBJECT
    protected:
    CommunicationDevice() = default;

    public:
    using Duration = std::chrono::steady_clock::duration;
    virtual ~CommunicationDevice() = default;

    virtual bool isConnected() = 0;
    virtual bool isConnecting() {
        return false;
    }
    virtual bool waitReceived(Duration timeout = std::chrono::seconds(1), int bytes = 1, bool isPolling = false) = 0;
    virtual bool waitReceived(Duration timeout, std::string escape_characters, std::string leading_pattern_indicating_skip_line) = 0;
    virtual void send(const QByteArray &data, const QByteArray &displayed_data = {}) = 0;
    void send(const std::vector<unsigned char> &data, const std::vector<unsigned char> &displayed_data = {});
    virtual void close() = 0;
    QString get_identifier_display_string() const;
    bool is_waiting_for_message() const;
    void set_currently_in_wait_received(bool in_wait_received);
    virtual QString getName() = 0;
    bool is_in_use() const;
    const QMap<QString, QVariant> &get_port_info();
    virtual bool connect(const QMap<QString, QVariant> &portinfo_) = 0;
    QString proposed_alias{};

    signals:
    void connected(const QByteArray &);
    void disconnected(const QByteArray &);
    void received(const QByteArray &);
    void sent(const QByteArray &);
    void decoded_sent(const QByteArray &);
    void decoded_received(const QByteArray &);
    void message(const QByteArray &);
    void read_ready();

    protected:
    bool currently_in_waitReceived;
    QMap<QString, QVariant> portinfo;
    friend void device_worker_set_in_use(CommunicationDevice *com_device, bool in_use);

    private:
    std::atomic<bool> in_use{false};
};

#endif // COMMUNICATIONDEVICE_H
