#include "scriptengine.h"
#include "config.h"
#include "console.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <regex>
#include <string>
#include <vector>

void ScriptEngine::load_script(const QString &path) {
	this->path = path;
	try {
		//lua.open_libraries(sol::lib::base); //load the standard lib if necessary
		lua.set_function("show_warning", [path](const std::string &title, const std::string &message) {
			QMessageBox::warning(nullptr, QString::fromStdString(title) + " from " + path, QString::fromStdString(message));
		});
		lua.script_file(path.toStdString());
	} catch (const sol::error &error) {
		set_error(error);
		throw;
	}
}

QStringList ScriptEngine::get_string_list(const QString &name) {
	QStringList retval;
	sol::table t = lua.get<sol::table>(name.toStdString());
	try {
		if (t.valid() == false) {
			return retval;
		}
		for (auto &s : t) {
			retval << s.second.as<std::string>().c_str();
		}
	} catch (const sol::error &error) {
		set_error(error);
		throw;
	}
	return retval;
}

void ScriptEngine::set_error(const sol::error &error) {
	const std::string &string = error.what();
	std::regex r(R"(\.lua:([0-9]*): )");
	std::smatch match;
	if (std::regex_search(string, match, r)) {
		Utility::convert(match[1].str(), error_line);
	}
}

void ScriptEngine::launch_editor() const {
	QStringList parameter;
	if (error_line != 0) {
		parameter << path + ":" + QString::number(error_line);
	} else {
		parameter << path;
	}
	QProcess::startDetached(QSettings{}.value(Globals::lua_editor_path_settings_key, R"(C:\Qt\Tools\QtCreator\bin\qtcreator.exe)").toString(), parameter);
}
