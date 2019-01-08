#ifndef SCRIPTSETUP_H
#define SCRIPTSETUP_H

#include <string>

namespace sol {
	class state;
}

class ScriptEngine;

void script_setup(sol::state &lua, const std::string &path, ScriptEngine &script_engine);

#endif // SCRIPTSETUP_H
