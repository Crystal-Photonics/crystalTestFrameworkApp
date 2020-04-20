#include "scpiprotocol_lua.h"
#include "communication_devices.h"
#include "exception_wrap.h"
#include "scpiprotocol.h"
#include "scriptsetup_helper.h"

void bind_scpiprotocol(sol::state &lua, ScriptEngine &script_engine) {
	lua.new_usertype<SCPIDevice>("SCPIDevice",                                                                                      //
								 sol::meta_function::construct, sol::no_constructor,                                                //
								 "get_protocol_name", wrap(&SCPIDevice::get_protocol_name),                                         //
								 "get_device_descriptor", wrap(&SCPIDevice::get_device_descriptor),                                 //
								 "get_str", wrap(&SCPIDevice::get_str),                                                             //
								 "get_str_param", wrap(&SCPIDevice::get_str_param),                                                 //
								 "get_num", exception_wrap(&script_engine, &SCPIDevice::get_num),                                   //
								 "get_num_param", exception_wrap(&script_engine, &SCPIDevice::get_num_param),                       //
								 "get_name", wrap(&SCPIDevice::get_name),                                                           //
								 "get_serial_number", wrap(&SCPIDevice::get_serial_number),                                         //
								 "get_manufacturer", wrap(&SCPIDevice::get_manufacturer),                                           //
								 "get_calibration", wrap(&SCPIDevice::get_calibration),                                             //
								 "is_event_received", wrap(&SCPIDevice::is_event_received),                                         //
								 "clear_event_list", wrap(&SCPIDevice::clear_event_list),                                           //
								 "get_event_list", wrap(&SCPIDevice::get_event_list),                                               //
								 "set_validation_max_standard_deviation", wrap(&SCPIDevice::set_validation_max_standard_deviation), //
								 "set_validation_retries", wrap(&SCPIDevice::set_validation_retries),                               //
								 "send_command", wrap(&SCPIDevice::send_command)                                                    //
	);
}
