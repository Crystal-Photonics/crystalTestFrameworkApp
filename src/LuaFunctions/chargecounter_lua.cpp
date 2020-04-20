#include "chargecounter_lua.h"
#include "chargecounter.h"
#include "scriptsetup_helper.h"

void bind_chargecounter(sol::state &lua) {
	lua.new_usertype<ChargeCounter>(
		"ChargeCounter", //
		sol::meta_function::construct, sol::factories(+[] {
			abort_check();
			return ChargeCounter{};
		}), //

		"add_current", wrap(&ChargeCounter::add_current), //
		"reset",
		+[](ChargeCounter &handle, const double current) {
			abort_check();
			(void)current;
			handle.reset();
		},                                                           //
		"get_current_hours", wrap(&ChargeCounter::get_current_hours) //
	);
}
