#include "communication_devices.h"

#include <sol.hpp>

sol::table SCPIDevice::get_device_descriptor() const {
	sol::table result = lua->create_table_with();
	protocol->get_lua_device_descriptor(result);
	return result;
}

sol::table SCPIDevice::get_str(std::string request) const {
	return protocol->get_str(*lua, request); //timeout possible
}

sol::table SCPIDevice::get_str_param(std::string request, std::string argument) const {
	return protocol->get_str_param(*lua, request, argument); //timeout possible
}

sol::table SCPIDevice::get_event_list() {
	return protocol->get_event_list(*lua);
}

sol::table SG04CountDevice::get_sg04_counts(bool clear) {
	return protocol->get_sg04_counts(*lua, clear);
}
