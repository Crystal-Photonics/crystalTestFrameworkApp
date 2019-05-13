#ifndef COMMUNICATION_DEVICES_H
#define COMMUNICATION_DEVICES_H

#include "Protocols/manualprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "Protocols/sg04countprotocol.h"

#include <sol_forward.hpp>
#include <string>

struct SCPIDevice {
	void send_command(std::string request) {
		protocol->send_command(request);
	}
	std::string get_protocol_name() const {
		return protocol->type.toStdString();
	}

	sol::table get_device_descriptor() const {
		sol::table result = lua->create_table_with();
		protocol->get_lua_device_descriptor(result);
		return result;
	}

	sol::table get_str(std::string request) const {
		return protocol->get_str(*lua, request); //timeout possible
	}

	sol::table get_str_param(std::string request, std::string argument) const {
		return protocol->get_str_param(*lua, request, argument); //timeout possible
	}

	double get_num(std::string request) const {
		return protocol->get_num(request); //timeout possible
	}

	double get_num_param(std::string request, std::string argument) const {
		return protocol->get_num_param(request, argument); //timeout possible
	}

	bool is_event_received(std::string event_name) const {
		return protocol->is_event_received(event_name);
	}

	void clear_event_list() {
		return protocol->clear_event_list();
	}

	sol::table get_event_list() {
		return protocol->get_event_list(*lua);
	}

	std::string get_name(void) const {
		return protocol->get_name();
	}

	std::string get_serial_number(void) const {
		return protocol->get_serial_number();
	}

	std::string get_manufacturer(void) const {
		return protocol->get_manufacturer();
	}
	std::string get_calibration(void) const {
		return protocol->get_approved_state_str().toStdString();
	}

	void set_validation_max_standard_deviation(double max_std_dev) {
		protocol->set_validation_max_standard_deviation(max_std_dev);
	}

	void set_validation_retries(unsigned int retries) {
		protocol->set_validation_retries(retries);
	}

	sol::state *lua = nullptr;
	SCPIProtocol *protocol = nullptr;
	CommunicationDevice *device = nullptr;
	ScriptEngine *engine = nullptr;
};

struct SG04CountDevice {
	std::string get_protocol_name() {
		return protocol->type.toStdString();
	}

	sol::table get_sg04_counts(bool clear) {
		return protocol->get_sg04_counts(*lua, clear);
	}

	uint accumulate_counts(uint time_ms) {
		return protocol->accumulate_counts(engine, time_ms);
	}

	sol::state *lua = nullptr;
	SG04CountProtocol *protocol = nullptr;
	CommunicationDevice *device = nullptr;
	ScriptEngine *engine = nullptr;
};

struct ManualDevice {
	std::string get_protocol_name() {
		return protocol->type.toStdString();
	}

	std::string get_name() {
		return protocol->get_name();
	}

	std::string get_manufacturer() {
		return protocol->get_manufacturer();
	}

	std::string get_description() {
		return protocol->get_description();
	}

	std::string get_serial_number() {
		return protocol->get_serial_number();
	}

	std::string get_notes() {
		return protocol->get_notes();
	}

	std::string get_summary() {
		return protocol->get_summary();
	}

	std::string get_calibration() {
		return protocol->get_approved_state_str().toStdString();
	}

	sol::state *lua = nullptr;
	ManualProtocol *protocol = nullptr;
	CommunicationDevice *device = nullptr;
	ScriptEngine *engine = nullptr;
};

#endif // COMMUNICATION_DEVICES_H
