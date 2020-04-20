#ifndef SCRIPTSETUP_H
#define SCRIPTSETUP_H

#include "console.h"

#include <string>

namespace sol {
    class state;
}

class ScriptEngine;
class UI_container;
class QPlainTextEdit;

void script_setup(sol::state &lua, const std::string &path, ScriptEngine &script_engine, UI_container *script_engine_parent,
				  QPlainTextEdit *script_engine_console_plaintext);

#endif // SCRIPTSETUP_H
