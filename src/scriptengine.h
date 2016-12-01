#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "export.h"
#include "sol.hpp"

#include <QFile>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

class ScriptEngine {
	public:
	void load_script(const QString &path);
	QStringList get_string_list(const QString &name);
	void launch_editor() const;

	private:
	void set_error(const sol::error &error);
	sol::state lua;
	QString path;
	int error_line = 0;
};

#endif // SCRIPTENGINE_H
