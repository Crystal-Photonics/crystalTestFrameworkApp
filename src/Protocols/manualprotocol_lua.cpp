#include "manualprotocol_lua.h"
#include "communication_devices.h"
#include "manualprotocol.h"
#include "scriptsetup_helper.h"

void bind_manualprotocol(sol::state &lua) {
	lua.new_usertype<ManualDevice>("ManualDevice",                                              //
								   sol::meta_function::construct, sol::no_constructor,          //
								   "get_protocol_name", wrap(&ManualDevice::get_protocol_name), //
								   "get_name", wrap(&ManualDevice::get_name),                   //
								   "get_manufacturer", wrap(&ManualDevice::get_manufacturer),   //
								   "get_description", wrap(&ManualDevice::get_description),     //
								   "get_serial_number", wrap(&ManualDevice::get_serial_number), //
								   "get_notes", wrap(&ManualDevice::get_notes),                 //
								   "get_calibration", wrap(&ManualDevice::get_calibration),     //
								   "get_summary", wrap(&ManualDevice::get_summary)              //
	);
}
