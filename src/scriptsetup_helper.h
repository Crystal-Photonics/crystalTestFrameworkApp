#ifndef SCRIPTSETUP_HELPER_H
#define SCRIPTSETUP_HELPER_H

#include "Windows/mainwindow.h"
#include "util.h"

#include <sol.hpp>

class UI_container;
class ScriptEngine;

template <class T>
struct Lua_UI_Wrapper {
	template <class... Args>
	Lua_UI_Wrapper(UI_container *parent, ScriptEngine *script_engine_to_terminate_on_exception, Args &&... args) {
		this->script_engine_to_terminate_on_exception = script_engine_to_terminate_on_exception;
		Utility::promised_thread_call(MainWindow::mw, [id = id, parent, args...] { MainWindow::mw->add_lua_UI_class<T>(id, parent, args...); });
	}
	Lua_UI_Wrapper(Lua_UI_Wrapper &&other)
		: id(other.id)
		, script_engine_to_terminate_on_exception{other.script_engine_to_terminate_on_exception} {
		other.id = -1;
	}
	Lua_UI_Wrapper &operator=(Lua_UI_Wrapper &&other) {
		std::swap(id, other.id);
	}
	~Lua_UI_Wrapper() {
		if (id != -1) {
			Utility::thread_call(
				MainWindow::mw, [id = this->id] { MainWindow::mw->remove_lua_UI_class<T>(id); }, script_engine_to_terminate_on_exception);
		}
	}

	int id = ++id_counter;

	private:
	static int id_counter;
	ScriptEngine *script_engine_to_terminate_on_exception = nullptr;
};

template <class T>
int Lua_UI_Wrapper<T>::id_counter;

void abort_check();

namespace detail {
	//this might be replacable by std::invoke once C++17 is available
	template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs, std::size_t... I>
	static ReturnType call_helper(ReturnType (UI_class::*func)(Args...), UI_class &ui, Params<ParamArgs...> const &params, std::index_sequence<I...>) {
		return (ui.*func)(std::get<I>(params)...);
	}
	template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs, std::size_t... I>
	static ReturnType call_helper(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, Params<ParamArgs...> const &params, std::index_sequence<I...>) {
		return (ui.*func)(std::get<I>(params)...);
	}

	template <class ReturnType, class UI_class, class... Args, class... ParamArgs>
	static ReturnType call(ReturnType (UI_class::*func)(Args...), UI_class &ui, std::tuple<ParamArgs...> const &params) {
		return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
	}
	template <class ReturnType, class UI_class, class... Args, class... ParamArgs>
	static ReturnType call(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, std::tuple<ParamArgs...> const &params) {
		return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
	}

	//list of types useful for returning a variadic template parameter pack or inspecting some of the parameters
	template <class... Args>
	struct Type_list;
	template <class Head_, class... Tail>
	struct Type_list<Head_, Tail...> {
		using Head = Head_;
		using Next = Type_list<Tail...>;
		static constexpr auto size = sizeof...(Tail) + 1;
	};
	template <>
	struct Type_list<> {
		static constexpr std::size_t size = 0;
	};

	//allows conversion from sol::object into int, std::string, ...
	struct Converter {
		Converter(sol::object object)
			: object{std::move(object)} {}
		sol::object object;
		template <class T>
		operator T() {
			return object.as<T>();
		}
	};

	//call a function with a list of sol::objects as the arguments
	template <class Function, std::size_t... indexes>
	static auto call(std::vector<sol::object> &objects, Function &&function, std::index_sequence<indexes...>) {
		return function(Converter{std::move(objects[indexes])}...);
	}

	//check if a list of sol::objects is convertible to a given variadic template parameter pack
	template <class... Args, std::size_t... indexes>
	static bool is_convertible(std::vector<sol::object> &objects, std::index_sequence<indexes...>) {
		return (... && objects[indexes].is<Args>());
	}
	template <class... Args>
	static bool is_convertible(Type_list<Args...>, std::vector<sol::object> &objects) {
		assert(sizeof...(Args) == objects.size());
		return is_convertible<Args...>(objects, std::index_sequence_for<Args...>());
	}

	//information for callable objects, such as number of parameters, parameter types and return type.
	//Fails to compile if the list is ambiguous due to overloading.
	template <class Return_type_, bool is_class_member_, class Parent_class_, class... Parameters_>
	struct Callable_info {
		using Return_type = Return_type_;
		using Parameters = Type_list<Parameters_...>;
		constexpr static bool is_class_member = is_class_member_;
		using Parent_class = Parent_class_;
	};
	template <class ReturnType, class... Args>
	Callable_info<ReturnType, false, void, Args...> callable_info(ReturnType (*)(Args...));
	template <class ReturnType, class Class, class... Args>
	Callable_info<ReturnType, true, Class, Args...> callable_info(ReturnType (Class::*)(Args...));
	template <class ReturnType, class Class, class... Args>
	Callable_info<ReturnType, true, Class, Args...> callable_info(ReturnType (Class::*)(Args...) const);
	template <class Function>
	auto callable_info(Function) -> decltype(callable_info(&Function::operator()));

	template <class Function>
	using Pointer_to_callable_t = std::add_pointer_t<std::remove_reference_t<Function>>;

	//get the parameter list of a callable object. Fails to compile if the list is ambiguous due to overloading.
	template <class Function>
	auto get_parameter_list(Function &&function) -> typename decltype(callable_info(function))::Parameters;
	template <class Function>
	using parameter_list_t = decltype(get_parameter_list(*Pointer_to_callable_t<Function>{}));

	//get the return type of a callable object. Fails to compile if the list is ambiguous due to overloading.
	template <class Function>
	auto get_return_type(Function &&function) -> typename decltype(callable_info(function))::Return_type;
	template <class Function>
	using return_type_t = decltype(get_return_type(*Pointer_to_callable_t<Function>{}));

	//get number of parameters of a callable. Fails to compile when ambiguous for overloaded callables.
	template <class Function>
	constexpr auto number_of_parameters{decltype(get_parameter_list(*Pointer_to_callable_t<Function>{}))::size};

	//call a function if it is callable with the given sol::objects
	template <class ReturnType, class FunctionHead, class... FunctionsTail>
	static ReturnType try_call(std::vector<sol::object> &objects, FunctionHead &&function, FunctionsTail &&... functions) {
		constexpr auto arity = detail::number_of_parameters<FunctionHead>;
		if (arity == objects.size() && is_convertible(parameter_list_t<FunctionHead>{}, objects)) {
			//right number of arguments and can convert between the argumens and parameters
			return static_cast<ReturnType>(call(objects, std::forward<FunctionHead>(function), std::make_index_sequence<arity>()));
		}
		//cannot call function, try with other functions
		if constexpr (sizeof...(FunctionsTail) == 0) {
			throw std::runtime_error("Invalid arguments for overloaded function call, none of the functions could handle the given arguments");
		} else {
			return try_call<ReturnType>(objects, std::forward<FunctionsTail>(functions)...);
		}
	}
} // namespace detail

//wrapper that wraps a UI function such as Button::has_been_clicked so that it is called from the main window context. Waits for processing.
template <class ReturnType, class UI_class, class... Args>
static auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...)) {
	return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
		return Utility::promised_thread_call(MainWindow::mw, [function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...)]() mutable {
			UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
			return detail::call(function, ui, std::move(args));
		});
	};
}
template <class ReturnType, class UI_class, class... Args>
static auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...) const) {
	return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
		return Utility::promised_thread_call(MainWindow::mw, [function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...)]() mutable {
			UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
			return detail::call(function, ui, std::move(args));
		});
	};
}

//wrapper that wraps a UI function so it is NOT called from the GUI thread. The function must not call any GUI-related functions in the non-GUI thread.
template <class ReturnType, class UI_class, class... Args>
static auto non_gui_call_wrapper(ReturnType (UI_class::*function)(Args...)) {
	return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
		UI_class &ui = Utility::promised_thread_call(MainWindow::mw, [id = lui.id]() -> UI_class & { return MainWindow::mw->get_lua_UI_class<UI_class>(id); });
		return (ui.*function)(std::forward<Args>(args)...);
	};
}

//wrapper that wraps a UI function such as Button::has_been_clicked so that it is called from the main window context. Doesn't wait for processing.
template <class ReturnType, class UI_class, class... Args>
static auto thread_call_wrapper_non_waiting(ScriptEngine *script_engine_to_terminate_on_exception, ReturnType (UI_class::*function)(Args...)) {
	return [function, script_engine_to_terminate_on_exception](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
		Utility::thread_call(
			MainWindow::mw,
			[function, id = lui.id, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
				UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
				return detail::call(function, ui, std::move(args));
			},
			script_engine_to_terminate_on_exception);
	};
}

template <class Return_type, class Class, class... Args>
auto wrap(Return_type (Class::*function)(Args... args)) { //wrapping member function
	return [function](Class *object, Args... args) {
		abort_check();
		return (object->*function)(std::forward<Args>(args)...);
	};
}

template <class Return_type, class Class, class... Args>
auto wrap(Return_type (Class::*function)(Args... args) const) { //wrapping const member function
	return [function](const Class *object, Args... args) {
		abort_check();
		return (object->*function)(std::forward<Args>(args)...);
	};
}

template <class Return_type, class... Args>
auto wrap(Return_type (*function)(Args... args)) { //wrapping non-member function
	return [function](Args... args) {
		abort_check();
		return function(std::forward<Args>(args)...);
	};
}

#endif // SCRIPTSETUP_HELPER_H
