#include "communication_logger.h"
#include "Protocols/rpcprotocol.h"
#include "Windows/devicematcher.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <iomanip>

Communication_logger::Communication_logger(Console &console) {
	//connections.push_back(connect(console, &QPlainTextEdit::));
}

Communication_logger::~Communication_logger() {
	for (auto &connection : connections) {
		disconnect(connection);
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

void Communication_logger::add(const MatchedDevice &device) {
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

	(*log_target) << "Info: Device " << device_count << " is " << device.device->get_identifier_display_string().toStdString() << " with protocol "
				  << device.protocol->type.toStdString() << ".\n";
	if (auto rpc_protocol = dynamic_cast<RPCProtocol *>(device.protocol)) {
		(*log_target) << "Info: Protocol version: " << rpc_protocol->get_description().get_version_number() << '\n';
	}

	for (const auto &comdata : communication_data) {
		connections.push_back(connect(device.device, comdata.signal, [ this, device = device_count, action = comdata.name ](const QByteArray &data) {
			(*log_target) << action << device << ": " << data << '\n';
		}));
	}
	device_count++;
}

void Communication_logger::set_log_file(const std::string &filepath) {
	logfile.close();
	logfile.open(filepath);
	if (not logfile.is_open()) {
		qDebug() << __PRETTY_FUNCTION__ << "failed to open logfile " << filepath.c_str() << "so log will not be saved to file";
	} else {
		logfile << log.str();
		log.str("");
		log_target = &logfile;
	}
}
