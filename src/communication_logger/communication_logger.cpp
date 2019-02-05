#include "communication_logger.h"
#include "Protocols/rpcprotocol.h"
#include "Windows/devicematcher.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <iomanip>

Communication_logger::Communication_logger(QPlainTextEdit *console) {}

Communication_logger::~Communication_logger() {
	for (auto &connection : connections) {
		disconnect(connection);
	}
}

void Communication_logger::set_file_path(const std::string &file_path) {
	file.open(file_path);
	if (not file.is_open()) {
		qDebug() << "Warning: Failed opening log file" << file_path.c_str();
	}
}

static std::ostream &operator<<(std::ostream &os, const QByteArray &data) {
	for (unsigned char c : data) {
		switch (c) {
			case '\n':
				os << c;
				break;
			case '\r':
				break;
			case '\t':
				os << c;
				break;
			default:
				if (c < 32 || c > 126) {
					os << '%' << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << static_cast<int>(c);
				} else {
					os << c;
				}
		}
	}
	return os;
}

void Communication_logger::add(MatchedDevice &device) {
	struct {
		void (CommunicationDevice::*signal)(const QByteArray &);
		const char *name;
		//TODO: color
	} communication_data[] = {{&CommunicationDevice::connected, "Connected "},
							  {&CommunicationDevice::disconnected, "Disconnected "},
							  {&CommunicationDevice::received, "< "},
							  {&CommunicationDevice::sent, "> "},
							  {&CommunicationDevice::decoded_received, "<<< "},
							  {&CommunicationDevice::decoded_sent, ">>> "},
							  {&CommunicationDevice::message, ": "}

	};

	file << "Info: Device " << device_count << " is " << device.device->get_identifier_display_string().toStdString() << " with protocol "
		 << device.protocol->type.toStdString() << ".\n";
	if (auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol)) {
		file << "Info: Protocol version: " << rpc_protocol->get_description().get_version_number() << '\n';
	}

	for (const auto &comdata : communication_data) {
		connections.push_back(connect(device.device, comdata.signal, [ this, device = device_count, action = comdata.name ](const QByteArray &data) {
			file << action << device << ": " << data << '\n';
		}));
	}
	device_count++;
}
