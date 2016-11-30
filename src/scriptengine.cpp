#include "scriptengine.h"
#include "console.h"
#include "sol.hpp"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <string>
#include <vector>

ScriptEngine::ScriptEngine() {
	lua = std::make_unique<sol::state>();
}

bool ScriptEngine::load_script(const QString &path) {
	return lua->load_file(path.toStdString());
}

void ScriptEngine::run_function(const QString &name, QStringList &retval) {
	try {
		sol::function f = (*lua)[name.toStdString()];
		auto result = f.call<std::vector<std::string>>();
		for (auto &r : result) {
			retval << r.c_str();
		}
	} catch (const sol::error &error) {
		Console::warning() << error.what();
	}
}

ScriptEngine::ScriptEngine(ScriptEngine &&other) {
	std::swap(lua, other.lua);
}

ScriptEngine &ScriptEngine::operator=(ScriptEngine &&other) {
	std::swap(lua, other.lua);
	return *this;
}

ScriptEngine::~ScriptEngine() {}
