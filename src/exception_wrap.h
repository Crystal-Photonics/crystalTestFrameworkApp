#ifndef EXCEPTION_WRAP_H
#define EXCEPTION_WRAP_H

#include "scriptsetup_helper.h"

QMessageBox::StandardButton ask_retry_abort_ignore(ScriptEngine *se, const std::exception &exception);

template <class Function>
static auto try_call(ScriptEngine *se, Function &&function) {
	for (;;) {
		abort_check();
		try {
			return function();
		} catch (const std::exception &exception) {
			switch (ask_retry_abort_ignore(se, exception)) {
				case QMessageBox::Abort:
					throw;
				case QMessageBox::Ignore:
					return decltype(function()){};
				default:
					continue;
			}
		}
	}
}

template <class Return_type, class Class, class... Args>
auto exception_wrap(ScriptEngine *se, Return_type (Class::*function)(Args... args)) { //wrapping member function
	return [function, se](Class *object, Args... args) { return try_call(se, [&] { return (object->*function)(args...); }); };
}

template <class Return_type, class Class, class... Args>
auto exception_wrap(ScriptEngine *se, Return_type (Class::*function)(Args... args) const) { //wrapping const member function
	return [function, se](const Class *object, Args... args) { return try_call(se, [&] { return (object->*function)(args...); }); };
}

template <class Return_type, class... Args>
auto exception_wrap(ScriptEngine *se, Return_type (*function)(Args... args)) { //wrapping non-member function
	return [function, se](Args... args) { return try_call([&] { return function(args...); }); };
}

#endif // EXCEPTION_WRAP_H
