#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "export.h"

#include <QFile>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

namespace sol {
	class state;
}

class ScriptEngine {
	public:
	explicit ScriptEngine();
	bool load_script(const QString &path);
	void run_function(const QString &name, QString &retval);
	void run_function(const QString &name, QStringList &retval);
	ScriptEngine(ScriptEngine &&other);
	ScriptEngine &operator=(ScriptEngine &&other);
	~ScriptEngine();

	private:
	std::unique_ptr<sol::state> lua;
};

#endif // SCRIPTENGINE_H
