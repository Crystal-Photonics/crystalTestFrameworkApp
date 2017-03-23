#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "sol.hpp"

#include <QDebug>
#include <QString>
#include <functional>
#include <memory>
#include <vector>
#include <QList>

class CommunicationDevice;
class QStringList;
class QSplitter;
class QPlainTextEdit;
struct Protocol;
class Data_engine;

class DeviceRequirements{
public:
	QString protocol_name;
	QStringList device_names;
	int quantity_min = 0;
	int quantity_max = INT_MAX;
	QString get_description() const;
};

class ScriptEngine {
	public:
	friend class TestRunner;
	friend class TestDescriptionLoader;
	friend class DeviceWorker;

	ScriptEngine(QSplitter *parent, QPlainTextEdit *console, Data_engine *data_engine);
	~ScriptEngine();
	void load_script(const QString &path);
	static void launch_editor(QString path, int error_line = 1);
	void launch_editor() const;
	static std::string to_string(double d);
	static std::string to_string(const sol::object &o);
	static std::string to_string(const sol::stack_proxy &object);

	private: //note: most of these things are private so that the GUI thread does not access anything important. Do not make things public.
	sol::table create_table();
	QStringList get_string_list(const QString &name);
	std::vector<DeviceRequirements> get_device_requirement_list(const QString &name);
	void run(std::vector<std::pair<CommunicationDevice *, Protocol *>> &devices);
	template <class ReturnType, class... Arguments>
	ReturnType call(const char *function_name, Arguments &&... args);

	void set_error(const sol::error &error);
	std::unique_ptr<sol::state> lua{};
	QString path{};
	int error_line{0};
	QSplitter *parent{nullptr};
	QPlainTextEdit *console{nullptr};
	Data_engine *data_engine{nullptr};
	std::unique_ptr<std::string> pdf_filepath{std::make_unique<std::string>()};
	std::unique_ptr<std::string> form_filepath{std::make_unique<std::string>()};
};

template <class ReturnType, class... Arguments>
ReturnType ScriptEngine::call(const char *function_name, Arguments &&... args) {
	sol::protected_function f = (*lua)[function_name];
	auto call_res = f(std::forward<Arguments>(args)...);
	if (call_res.valid()) {
		return call_res;
	}
	sol::error error = call_res;
	set_error(error);
	throw error;
}

#endif // SCRIPTENGINE_H
