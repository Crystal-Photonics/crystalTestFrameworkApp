#include "sg04countprotocol_lua.h"
#include "communication_devices.h"
#include "scriptsetup_helper.h"
#include "sg04countprotocol.h"

void bind_sg04countprotocol(sol::state &lua) {
	lua.new_usertype<SG04CountDevice>(
		"SG04CountDevice",                                              //
		sol::meta_function::construct, sol::no_constructor,             //
		"get_protocol_name", wrap(&SG04CountDevice::get_protocol_name), //
		"get_name",
		+[](SG04CountDevice &protocol) {
			abort_check();
			(void)protocol;
			return "SG04";
		},                                                             //
		"get_sg04_counts", wrap(&SG04CountDevice::get_sg04_counts),    //
		"accumulate_counts", wrap(&SG04CountDevice::accumulate_counts) //
	);
}
