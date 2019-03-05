#include "scriptsetup.h"
#include "LuaUI/button.h"
#include "LuaUI/checkbox.h"
#include "LuaUI/color.h"
#include "LuaUI/combobox.h"
#include "LuaUI/combofileselector.h"
#include "LuaUI/dataengineinput.h"
#include "LuaUI/hline.h"
#include "LuaUI/image.h"
#include "LuaUI/isotopesourceselector.h"
#include "LuaUI/label.h"
#include "LuaUI/lineedit.h"
#include "LuaUI/plot.h"
#include "LuaUI/polldataengine.h"
#include "LuaUI/progressbar.h"
#include "LuaUI/spinbox.h"
#include "LuaUI/userinstructionlabel.h"
#include "LuaUI/userwaitlabel.h"
#include "Protocols/manualprotocol.h"
#include "Protocols/scpiprotocol.h"
#include "Protocols/sg04countprotocol.h"
#include "Windows/devicematcher.h"
#include "Windows/mainwindow.h"
#include "chargecounter.h"
#include "communication_devices.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h"
#include "datalogger.h"
#include "environmentvariables.h"
#include "lua_functions.h"
#include "qt_util.h"
#include "scriptengine.h"
#include "sol.hpp"
#include "ui_container.h"

#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QString>
#include <QThread>
#include <fstream>

struct Data_engine_handle {
    Data_engine *data_engine{nullptr};
    Data_engine_handle() = delete;
};

template <class T>
struct Lua_UI_Wrapper {
    template <class... Args>
    Lua_UI_Wrapper(UI_container *parent, ScriptEngine *script_engine_to_terminate_on_exception, Args &&... args) {
        this->script_engine_to_terminate_on_exception = script_engine_to_terminate_on_exception;
        Utility::thread_call(MainWindow::mw, [ id = id, parent, args... ] { MainWindow::mw->add_lua_UI_class<T>(id, parent, args...); },
                             script_engine_to_terminate_on_exception);
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
            Utility::thread_call(MainWindow::mw, [id = this->id] { MainWindow::mw->remove_lua_UI_class<T>(id); }, script_engine_to_terminate_on_exception);
        }
    }

    int id = ++id_counter;

    private:
    static int id_counter;
    ScriptEngine *script_engine_to_terminate_on_exception = nullptr;
};

template <class T>
int Lua_UI_Wrapper<T>::id_counter;

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

    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs>
	static ReturnType call(ReturnType (UI_class::*func)(Args...), UI_class &ui, Params<ParamArgs...> const &params) {
        return call_helper(func, ui, params, std::index_sequence_for<Args...>{});
    }
    template <class ReturnType, class UI_class, class... Args, template <class...> class Params, class... ParamArgs>
	static ReturnType call(ReturnType (UI_class::*func)(Args...) const, UI_class &ui, Params<ParamArgs...> const &params) {
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
        static constexpr auto size = 0;
    };

    //allows conversion from sol::object into int, std::string, ...
    struct Converter {
        Converter(sol::object object)
            : object{object} {}
        sol::object object;
        template <class T>
        operator T() {
            return object.as<T>();
        }
    };

    //call a function with a list of sol::objects as the arguments
    template <class Function, std::size_t... indexes>
	static auto call(std::vector<sol::object> &objects, Function &&function, std::index_sequence<indexes...>) {
        return function(Converter(objects[indexes])...);
    }

    //check if a list of sol::objects is convertible to a given variadic template parameter pack
    template <int index>
	static bool is_convertible(std::vector<sol::object> &) {
        return true;
    }
    template <int index, class Head, class... Tail>
	static bool is_convertible(std::vector<sol::object> &objects) {
        if (objects[index].is<Head>()) {
            return is_convertible<index + 1, Tail...>(objects);
        }
        return false;
    }
    template <class... Args>
	static bool is_convertible(Type_list<Args...>, std::vector<sol::object> &objects) {
        return is_convertible<0, Args...>(objects);
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
    template <class ReturnType>
	static ReturnType try_call(std::false_type /*return void*/, std::vector<sol::object> &) {
        throw std::runtime_error("Invalid arguments for overloaded function call, none of the functions could handle the given arguments");
    }
    template <class ReturnType>
	static void try_call(std::true_type /*return void*/, std::vector<sol::object> &) {
        throw std::runtime_error("Invalid arguments for overloaded function call, none of the functions could handle the given arguments");
    }
    template <class ReturnType, class FunctionHead, class... FunctionsTail>
	static ReturnType try_call(std::false_type /*return void*/, std::vector<sol::object> &objects, FunctionHead &&function, FunctionsTail &&... functions) {
        constexpr auto arity = detail::number_of_parameters<FunctionHead>;
        //skip function if it has the wrong number of parameters
        if (arity != objects.size()) {
            return try_call<ReturnType>(std::false_type{}, objects, std::forward<FunctionsTail>(functions)...);
        }
        //skip function if the argument types don't match
        if (is_convertible(parameter_list_t<FunctionHead>{}, objects)) {
            return call(objects, std::forward<FunctionHead>(function), std::make_index_sequence<arity>());
        }
        //give up and try the next overload
        return try_call<ReturnType>(std::false_type{}, objects, functions...);
    }
    template <class ReturnType, class FunctionHead, class... FunctionsTail>
	static void try_call(std::true_type /*return void*/, std::vector<sol::object> &objects, FunctionHead &&function, FunctionsTail &&... functions) {
        constexpr auto arity = detail::number_of_parameters<FunctionHead>;
        //skip function if it has the wrong number of parameters
        if (arity != objects.size()) {
            return try_call<ReturnType>(std::true_type{}, objects, std::forward<FunctionsTail>(functions)...);
        }
        //skip function if the argument types don't match
        if (is_convertible(parameter_list_t<FunctionHead>{}, objects)) {
            call(objects, std::forward<FunctionHead>(function), std::make_index_sequence<arity>());
            return;
        }
        //give up and try the next overload
        return try_call<ReturnType>(std::true_type{}, objects, functions...);
    }

    template <class ReturnType, class... Functions>
	static auto overloaded_function_helper(std::false_type /*should_returntype_be_deduced*/, Functions &&... functions) {
        return [functions...](sol::variadic_args args) {
            std::vector<sol::object> objects;
            for (auto object : args) {
                objects.push_back(std::move(object));
            }
            return try_call<ReturnType>(std::is_same<ReturnType, void>(), objects, functions...);
        };
    }

    template <class ReturnType, class Functions_head, class... Functions_tail>
	static auto overloaded_function_helper(std::true_type /*should_returntype_be_deduced*/, Functions_head &&functions_head,
										   Functions_tail &&... functions_tail) {
        return overloaded_function_helper<return_type_t<Functions_head>>(std::false_type{}, std::forward<Functions_head>(functions_head),
                                                                         std::forward<Functions_tail>(functions_tail)...);
    }
} // namespace detail

static void abort_check() {
	if (QThread::currentThread()->isInterruptionRequested()) {
		throw sol::error("Abort Requested");
	}
}

//wrapper that wraps a UI function such as Button::has_been_clicked so that it is called from the main window context. Waits for processing.
template <class ReturnType, class UI_class, class... Args>
static auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...)) {
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
        //TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        return Utility::promised_thread_call(MainWindow::mw, [ function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        });
    };
}
template <class ReturnType, class UI_class, class... Args>
static auto thread_call_wrapper(ReturnType (UI_class::*function)(Args...) const) {
    return [function](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
        //TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        return Utility::promised_thread_call(MainWindow::mw, [ function, id = lui.id, args = std::forward_as_tuple(std::forward<Args>(args)...) ]() mutable {
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
		//TODO: Decide if we should use promised_thread_call or thread_call
        //promised_thread_call lets us get return values while thread_call does not
        //however, promised_thread_call hangs if the gui thread hangs while thread_call does not
        //using thread_call iff ReturnType is void and promised_thread_call otherwise requires some more template magic
        UI_class &ui = Utility::promised_thread_call(MainWindow::mw, [id = lui.id]()->UI_class & { return MainWindow::mw->get_lua_UI_class<UI_class>(id); });
        return (ui.*function)(args...);
    };
}

//wrapper that wraps a UI function such as Button::has_been_clicked so that it is called from the main window context. Doesn't wait for processing.
template <class ReturnType, class UI_class, class... Args>
static auto thread_call_wrapper_non_waiting(ScriptEngine *script_engine_to_terminate_on_exception, ReturnType (UI_class::*function)(Args...)) {
    return [function, script_engine_to_terminate_on_exception](Lua_UI_Wrapper<UI_class> &lui, Args &&... args) {
		abort_check();
		Utility::thread_call(MainWindow::mw, [ function, id = lui.id, args = std::make_tuple(std::forward<Args>(args)...) ]() mutable {
            UI_class &ui = MainWindow::mw->get_lua_UI_class<UI_class>(id);
            return detail::call(function, ui, std::move(args));
        },
							 script_engine_to_terminate_on_exception);
    };
}

//create an overloaded function from a list of functions. When called the overloaded function will pick one of the given functions based on arguments.
template <class ReturnType = std::false_type, class... Functions>
static auto overloaded_function(Functions &&... functions) {
    return detail::overloaded_function_helper<ReturnType>(typename std::is_same<ReturnType, std::false_type>::type{}, std::forward<Functions>(functions)...);
}

void script_setup(sol::state &lua, const std::string &path, ScriptEngine &script_engine) {
    //load the standard libs if necessary
    lua.open_libraries();

    EnvironmentVariables env_variables(QSettings{}.value(Globals::path_to_environment_variables_key, "").toString());
    env_variables.load_to_lua(&lua);

    //add generic function
    {
		lua["show_file_save_dialog"] = [path](const std::string &title, const std::string &preselected_path, sol::table filters) {
			abort_check();
            return show_file_save_dialog(title, get_absolute_file_path(QString::fromStdString(path), preselected_path), filters);
		};
		lua["show_question"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message, sol::table button_table) {
			abort_check();
			return show_question(QString::fromStdString(path), title, message, button_table);
		};

		lua["show_info"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
			abort_check();
			show_info(QString::fromStdString(path), title, message);
		};

		lua["show_warning"] = [path](const sol::optional<std::string> &title, const sol::optional<std::string> &message) {
			abort_check();
			show_warning(QString::fromStdString(path), title, message);
		};

		lua["print"] = [console = script_engine.console.get_plaintext_edit()](const sol::variadic_args &args) {
			abort_check();
			print(console, args);
		};

		lua["sleep_ms"] = [&script_engine](const unsigned int timeout_ms) {
			abort_check();
			sleep_ms(&script_engine, timeout_ms);
		};
		lua["pc_speaker_beep"] = +[]() {
			abort_check();
			pc_speaker_beep();
		};
		lua["current_date_time_ms"] = +[] {
			abort_check();
			return current_date_time_ms();
		};
		lua["round"] = +[](const double value, const unsigned int precision = 0) {
			abort_check();
			return round_double(value, precision);
		};
		lua["require"] = [ path = path, &lua ](const std::string &file) {
			abort_check();
            QDir dir(QString::fromStdString(path));
            dir.cdUp();
            lua.script_file(dir.absoluteFilePath(QString::fromStdString(file) + ".lua").toStdString());
		};
		lua["await_hotkey"] = [&script_engine] {
			abort_check();
			auto exit_value = script_engine.await_hotkey_event();

            switch (exit_value) {
				case Event_id::Hotkey_confirm_pressed:
                    return "confirm";
                case Event_id::Hotkey_skip_pressed:
                    return "skip";
                case Event_id::Hotkey_cancel_pressed:
                    return "cancel";
                default: { throw sol::error("interrupted"); }
            }
		};
		lua["discover_devices"] = [&script_engine](const sol::table &device_description) {
			abort_check();
			const auto &devices = MainWindow::mw->discover_devices(script_engine, device_description);
			for (const auto &device : devices) {
				script_engine.adopt_device(device);
			}
			return script_engine.get_devices(devices);
		};
		lua["refresh_devices"] = +[] {
			abort_check();
			Utility::thread_call(MainWindow::mw, +[] {
				MainWindow::mw->on_actionrefresh_devices_all_triggered(); //does not wait for devices to be refreshed, so we wait for 2 seconds.
			});
			std::this_thread::sleep_for(std::chrono::seconds(2));
		};
		lua["refresh_DUTs"] = +[] {
			abort_check();
			Utility::thread_call(MainWindow::mw, +[] {
				MainWindow::mw->on_actionrefresh_devices_dut_triggered(); //does not wait for devices to be refreshed, so we wait for 2 seconds.
			});
			std::this_thread::sleep_for(std::chrono::seconds(2));
		};
    }

    //table functions
    {
		lua["table_save_to_file"] =
			[ console = script_engine.console.get_plaintext_edit(), path = path ](const std::string file_name, sol::table input_table, bool over_write_file) {
			abort_check();
			table_save_to_file(console, get_absolute_file_path(QString::fromStdString(path), file_name), input_table, over_write_file);
		};
		lua["table_load_from_file"] = [&lua, console = script_engine.console.get_plaintext_edit(), path = path ](const std::string file_name) {
			abort_check();
			return table_load_from_file(console, lua, get_absolute_file_path(QString::fromStdString(path), file_name));
		};
		lua["table_sum"] = [](sol::table table) {
			abort_check();
			return table_sum(table);
		};

		lua["table_crc16"] = [console = script_engine.console.get_plaintext_edit()](sol::table table) {
			abort_check();
			return table_crc16(console, table);
		};

		lua["table_mean"] = [](sol::table table) {
			abort_check();
			return table_mean(table);
		};

		lua["table_variance"] = [](sol::table table) {
			abort_check();
			return table_variance(table);
		};

		lua["table_standard_deviation"] = [](sol::table table) {
			abort_check();
			return table_standard_deviation(table);
		};

		lua["table_set_constant"] = [&lua](sol::table input_values, double constant) {
			abort_check();
			return table_set_constant(lua, input_values, constant);
		};

		lua["table_create_constant"] = [&lua](const unsigned int size, double constant) {
			abort_check();
			return table_create_constant(lua, size, constant);
		};

		lua["table_add_table"] = [&lua](sol::table input_values_a, sol::table input_values_b) {
			abort_check();
			return table_add_table(lua, input_values_a, input_values_b);
		};

		lua["table_add_table_at"] = [&lua](sol::table input_values_a, sol::table input_values_b, unsigned int at) {
			abort_check();
			return table_add_table_at(lua, input_values_a, input_values_b, at);
		};

		lua["table_add_constant"] = [&lua](sol::table input_values, double constant) {
			abort_check();
			return table_add_constant(lua, input_values, constant);
		};

		lua["table_sub_table"] = [&lua](sol::table input_values_a, sol::table input_values_b) {
			abort_check();
			return table_sub_table(lua, input_values_a, input_values_b);
		};

		lua["table_mul_table"] = [&lua](sol::table input_values_a, sol::table input_values_b) {
			abort_check();
			return table_mul_table(lua, input_values_a, input_values_b);
		};

		lua["table_mul_constant"] = [&lua](sol::table input_values_a, double constant) {
			abort_check();
			return table_mul_constant(lua, input_values_a, constant);
		};

		lua["table_div_table"] = [&lua](sol::table input_values_a, sol::table input_values_b) {
			abort_check();
			return table_div_table(lua, input_values_a, input_values_b);
		};

		lua["table_round"] = [&lua](sol::table input_values, const unsigned int precision = 0) {
			abort_check();
			return table_round(lua, input_values, precision);
		};

		lua["table_abs"] = [&lua](sol::table input_values) {
			abort_check();
			return table_abs(lua, input_values);
		};

		lua["table_mid"] = [&lua](sol::table input_values, const unsigned int start, const unsigned int length) {
			abort_check();
			return table_mid(lua, input_values, start, length);
		};

		lua["table_equal_constant"] = [](sol::table input_values_a, double input_const_val) {
			abort_check();
			return table_equal_constant(input_values_a, input_const_val);
		};

		lua["table_equal_table"] = [](sol::table input_values_a, sol::table input_values_b) {
			abort_check();
			return table_equal_table(input_values_a, input_values_b);
		};

		lua["table_max"] = [](sol::table input_values) {
			abort_check();
			return table_max(input_values);
		};

		lua["table_min"] = [](sol::table input_values) {
			abort_check();
			return table_min(input_values);
		};

		lua["table_max_abs"] = [](sol::table input_values) {
			abort_check();
			return table_max_abs(input_values);
		};

		lua["table_min_abs"] = [](sol::table input_values) {
			abort_check();
			return table_min_abs(input_values);
		};

		lua["table_max_by_field"] = [&lua](sol::table input_values, const std::string field_name) {
			abort_check();
			return table_max_by_field(lua, input_values, field_name);
		};

#if 1
		lua["table_min_by_field"] = [&lua](sol::table input_values, const std::string field_name) {
			abort_check();
			return table_min_by_field(lua, input_values, field_name);
		};

		lua["propose_unique_filename_by_datetime"] = [path = path](const std::string &dir_path, const std::string &prefix, const std::string &suffix) {
			abort_check();
			return propose_unique_filename_by_datetime(get_absolute_file_path(QString::fromStdString(path), dir_path), prefix, suffix);
		};
		lua["git_info"] = [&lua, path = path ](std::string dir_path, bool allow_modified) {
			abort_check();
			return git_info(lua, get_absolute_file_path(QString::fromStdString(path), dir_path), allow_modified);
		};
		lua["run_external_tool"] = [path = path](const std::string &execute_directory, const std::string &executable, const sol::table &arguments,
												 uint timeout_s) {
			abort_check();
			return run_external_tool(QString::fromStdString(path),
									 QString::fromStdString(get_absolute_file_path(QString::fromStdString(path), execute_directory)),
									 QString::fromStdString(executable), arguments, timeout_s)
				.toStdString();
		};

#endif

		lua["get_framework_git_hash"] = []() {
			abort_check();
			return get_framework_git_hash();
		};
		lua["get_framework_git_date_unix"] = []() {
			abort_check();
			return get_framework_git_date_unix();
		};
		lua["get_framework_git_date_text"] = []() {
			abort_check();
			return get_framework_git_date_text();
		};

		lua["get_os_username"] = []() {
			abort_check();
			return get_os_username();
		};
    }

    {
		lua["measure_noise_level_czt"] = [&lua](sol::table rpc_device, const unsigned int dacs_quantity, const unsigned int max_possible_dac_value) {
			abort_check();
			return measure_noise_level_czt(lua, rpc_device, dacs_quantity, max_possible_dac_value);
		};
    }
    //bind DataLogger
    {
        lua.new_usertype<DataLogger>(
            "DataLogger", //
			sol::meta_function::construct, sol::factories([ console = script_engine.console.get_plaintext_edit(), path = path ](
                                               const std::string &file_name, char seperating_character, sol::table field_names, bool over_write_file) {
				abort_check();
				return DataLogger{console, get_absolute_file_path(QString::fromStdString(path), file_name), seperating_character, field_names, over_write_file};
            }), //

            "append_data",
			[](DataLogger &handle, const sol::table &data_record) {
				abort_check();
				return handle.append_data(data_record);
			}, //
			"save",
			[](DataLogger &handle) {
				abort_check();
				handle.save();
			} //
            );
    }
    //bind charge counter
    {
		lua.new_usertype<ChargeCounter>("ChargeCounter", //
										sol::meta_function::construct, sol::factories([]() {
											abort_check();
											return ChargeCounter{};
										}), //

										"add_current",
										[](ChargeCounter &handle, const double current) {
											abort_check();
											return handle.add_current(current);
										}, //
                                        "reset",
										[](ChargeCounter &handle, const double current) {
											abort_check();
											(void)current;
                                            handle.reset();
                                        }, //
                                        "get_current_hours",
										[](ChargeCounter &handle) {
											abort_check();
											return handle.get_current_hours();
										} //
                                        );
    }

    //bind data engine
    {
        lua.new_usertype<Data_engine_handle>(
            "Data_engine", //
            sol::meta_function::construct,
            sol::factories([ path = path, &script_engine ](const std::string &pdf_template_file, const std::string &json_file,
                                                           const std::string &auto_json_dump_path, const sol::table &dependency_tags) {
				abort_check();
				auto file_path = get_absolute_file_path(QString::fromStdString(path), json_file);
                auto xml_file = get_absolute_file_path(QString::fromStdString(path), pdf_template_file);
                auto auto_dump_path = get_absolute_file_path(QString::fromStdString(path), auto_json_dump_path);
                std::ifstream f(file_path);
                if (!f) {
                    throw std::runtime_error("Failed opening file " + file_path);
                }

				auto add_value_to_tag_list = [](QList<QVariant> &values, const sol::object &obj, const std::string &tag_name) {
                    if (obj.get_type() == sol::type::string) {
                        values.append(QString::fromStdString(obj.as<std::string>()));
                    } else if (obj.get_type() == sol::type::number) {
                        QVariant v;
                        v.setValue<double>(obj.as<double>());
                        values.append(v);
                    } else if (obj.get_type() == sol::type::boolean) {
                        QVariant v;
                        v.setValue<bool>(obj.as<bool>());
                        values.append(v);
                    } else {
                        throw std::runtime_error(
                            QString("invalid type in field of dependency tags at index %1").arg(QString::fromStdString(tag_name)).toStdString());
                    }
                };

                QMap<QString, QList<QVariant>> tags;
                for (auto &tag : dependency_tags) {
                    std::string tag_name = tag.first.as<std::string>();
                    QList<QVariant> values;

                    if (tag.second.get_type() == sol::type::table) {
                        const auto &value_list = tag.second.as<sol::table>();
                        for (size_t i = 1; i <= value_list.size(); i++) {
                            const sol::object &obj = value_list[i].get<sol::object>();
                            add_value_to_tag_list(values, obj, tag_name);
                        }
                    } else {
                        add_value_to_tag_list(values, tag.second, tag_name);
                    }

                    tags.insert(QString::fromStdString(tag_name), values);
                }
                script_engine.data_engine->set_dependancy_tags(tags);
                script_engine.data_engine->set_source(f);
                script_engine.data_engine->set_source_path(QString::fromStdString(file_path));
                script_engine.data_engine_pdf_template_path = QString::fromStdString(xml_file);
                script_engine.data_engine_auto_dump_path = QString::fromStdString(auto_dump_path);
                //*pdf_filepath = QDir{QSettings{}.value(Globals::form_directory, "").toString()}.absoluteFilePath("test_dump.pdf").toStdString();
                //*form_filepath = get_absolute_file_path(path, xml_file);

                return Data_engine_handle{script_engine.data_engine};
            }), //
            "set_start_time_seconds_since_epoch",
			[](Data_engine_handle &handle, const double start_time) {
				abort_check();
				return handle.data_engine->set_start_time_seconds_since_epoch(start_time);
			},
            "use_instance",
			[](Data_engine_handle &handle, const std::string &section_name, const std::string &instance_caption, const uint instance_index) {
				abort_check();
				handle.data_engine->use_instance(QString::fromStdString(section_name), QString::fromStdString(instance_caption), instance_index);
            },
            "start_recording_actual_value_statistic",
			[](Data_engine_handle &handle, const std::string &root_file_path, const std::string &prefix) {
				abort_check();
				return handle.data_engine->start_recording_actual_value_statistic(root_file_path, prefix);
            },
            "set_dut_identifier",
			[](Data_engine_handle &handle, const std::string &dut_identifier) {
				abort_check();
				return handle.data_engine->set_dut_identifier(QString::fromStdString(dut_identifier));
            },

            "get_instance_count",
			[](Data_engine_handle &handle, const std::string &section_name) {
				abort_check();
				return handle.data_engine->get_instance_count(section_name);
			},

            "get_description",
			[](Data_engine_handle &handle, const std::string &id) {
				abort_check();
				return handle.data_engine->get_description(QString::fromStdString(id)).toStdString();
			},
            "get_actual_value",
			[](Data_engine_handle &handle, const std::string &id) {
				abort_check();
				return handle.data_engine->get_actual_value(QString::fromStdString(id)).toStdString();
			},
            "get_actual_number",
			[](Data_engine_handle &handle, const std::string &id) {
				abort_check();
				return handle.data_engine->get_actual_number(QString::fromStdString(id));
			},

            "get_unit",
			[](Data_engine_handle &handle, const std::string &id) {
				abort_check();
				return handle.data_engine->get_unit(QString::fromStdString(id)).toStdString();
			},
            "get_desired_value",
			[](Data_engine_handle &handle, const std::string &id) {
				abort_check();
				return handle.data_engine->get_desired_value_as_string(QString::fromStdString(id)).toStdString();
            },
			"get_section_names",
			[&lua](Data_engine_handle &handle) {
				abort_check();
				return handle.data_engine->get_section_names(&lua);
			}, //
            "get_ids_of_section",
			[&lua](Data_engine_handle &handle, const std::string &section_name) {
				abort_check();
				return handle.data_engine->get_ids_of_section(&lua, section_name);
			},
            "set_instance_count",
			[](Data_engine_handle &handle, const std::string &instance_count_name, const uint instance_count) {
				abort_check();
				handle.data_engine->set_instance_count(QString::fromStdString(instance_count_name), instance_count);
            },
            "save_to_json",
            [path = path](Data_engine_handle & handle, const std::string &file_name) {
				abort_check();
				auto fn = get_absolute_file_path(QString::fromStdString(path), file_name);
                handle.data_engine->save_to_json(QString::fromStdString(fn));
            },
            "add_extra_pdf_path", [ path = path, &script_engine ](Data_engine_handle & handle, const std::string &file_name) {
				abort_check();
				(void)handle;
                // data_engine_auto_dump_path = QString::fromStdString(auto_dump_path);
                script_engine.additional_pdf_path = QString::fromStdString(get_absolute_file_path(QString::fromStdString(path), file_name));
                //  handle.data_engine->save_to_json(QString::fromStdString(fn));
            },
            "set_open_pdf_on_pdf_creation",
            [path = path](Data_engine_handle & handle, bool auto_open_on_pdf_creation) {
				abort_check();
				handle.data_engine->set_enable_auto_open_pdf(auto_open_on_pdf_creation);
            },
            "value_in_range",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->value_in_range(QString::fromStdString(field_id));
			},
            "value_in_range_in_section",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->value_in_range_in_section(QString::fromStdString(field_id));
            },
            "value_in_range_in_instance",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->value_in_range_in_instance(QString::fromStdString(field_id));
            },
            "values_in_range",
			[](Data_engine_handle &handle, const sol::table &field_ids) {
				abort_check();
				QList<FormID> ids;
                for (const auto &field_id : field_ids) {
                    ids.append(QString::fromStdString(field_id.second.as<std::string>()));
                }
                return handle.data_engine->values_in_range(ids);
            },
			"is_bool",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->is_bool(QString::fromStdString(field_id));
			},
			"is_text",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->is_text(QString::fromStdString(field_id));
			},
            "is_number",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->is_number(QString::fromStdString(field_id));
			},
            "is_exceptionally_approved",
			[](Data_engine_handle &handle, const std::string &field_id) {
				abort_check();
				return handle.data_engine->is_exceptionally_approved(QString::fromStdString(field_id));
            },
			"set_actual_number",
			[](Data_engine_handle &handle, const std::string &field_id, double value) {
				abort_check();
				handle.data_engine->set_actual_number(QString::fromStdString(field_id), value);
			},
			"set_actual_bool",
			[](Data_engine_handle &handle, const std::string &field_id, bool value) {
				abort_check();
				handle.data_engine->set_actual_bool(QString::fromStdString(field_id), value);
			},
#if 1
			"set_actual_datetime",
			[](Data_engine_handle &handle, const std::string &field_id, double value) {
				abort_check();
				handle.data_engine->set_actual_datetime(QString::fromStdString(field_id), DataEngineDateTime{value});
			},
#endif
#if 1
            "set_actual_datetime_from_text",
			[](Data_engine_handle &handle, const std::string &field_id, const std::string &text) {
				abort_check();
				handle.data_engine->set_actual_datetime(QString::fromStdString(field_id), DataEngineDateTime{QString::fromStdString(text)});
            },
#endif
            "set_actual_text",
			[](Data_engine_handle &handle, const std::string &field_id, const std::string &text) {
				abort_check();
				handle.data_engine->set_actual_text(QString::fromStdString(field_id), QString::fromStdString(text));
            },
			"all_values_in_range", [](Data_engine_handle &handle) { return handle.data_engine->all_values_in_range(); });
    }

    //bind UI
    auto ui_table = lua.create_named_table("Ui");

    //UI functions
    {
		ui_table["set_column_count"] = [ container = script_engine.parent, &script_engine ](int count) {
			abort_check();
			Utility::thread_call(MainWindow::mw, [container, count] { container->set_column_count(count); }, &script_engine);
		};
#if 0
		ui_table["load_user_entry_cache"] = [ container = parent, &script_engine ](const std::string dut_id) {
			abort_check();
			(void)container;
			(void)&script_engine;
			(void)dut_id;
			// container->user_entry_cache.load_storage_for_script(script_engine.path_m, QString::fromStdString(dut_id));
		};
#endif
    }

#if 0
	//bind charge UserEntryCache
	{
		lua.new_usertype<UserEntryCache>("UserEntryCache", //
										  sol::meta_function::construct,sol::factories(
										  []() { //
											  abort_check();
											  return UserEntryCache{};
										  }));
	}
#endif
    //bind plot
    {
        ui_table.new_usertype<Lua_UI_Wrapper<Curve>>(
            "Curve",                                                                               //
            sol::meta_function::construct, sol::no_constructor,                                    //
            "append_point", thread_call_wrapper_non_waiting(&script_engine, &Curve::append_point), //
            "add_spectrum",
            [&script_engine](Lua_UI_Wrapper<Curve> &curve, sol::table table) {
				abort_check();
				std::vector<double> data;
                data.reserve(table.size());
                for (auto &i : table) {
                    data.push_back(i.second.as<double>());
                }
                Utility::thread_call(MainWindow::mw, [ id = curve.id, data = std::move(data) ] {
                    auto &curve = MainWindow::mw->get_lua_UI_class<Curve>(id);
                    curve.add(data);
                },
                                     &script_engine);
            }, //
            "add_spectrum_at",
            [&script_engine](Lua_UI_Wrapper<Curve> &curve, const unsigned int spectrum_start_channel, const sol::table &table) {
				abort_check();
				std::vector<double> data;
                data.reserve(table.size());
                for (auto &i : table) {
                    data.push_back(i.second.as<double>());
                }
                Utility::thread_call(MainWindow::mw, [ id = curve.id, data = std::move(data), spectrum_start_channel ] {
                    auto &curve = MainWindow::mw->get_lua_UI_class<Curve>(id);
                    curve.add_spectrum_at(spectrum_start_channel, data);
                },
                                     &script_engine);
            }, //

            "clear",
            thread_call_wrapper(&Curve::clear),                                            //
            "set_median_enable", thread_call_wrapper(&Curve::set_median_enable),           //
            "set_median_kernel_size", thread_call_wrapper(&Curve::set_median_kernel_size), //
            "integrate_ci", thread_call_wrapper(&Curve::integrate_ci),                     //
            "set_x_axis_gain", thread_call_wrapper(&Curve::set_x_axis_gain),               //
            "set_x_axis_offset", thread_call_wrapper(&Curve::set_x_axis_offset),           //
            "get_y_values_as_array", non_gui_call_wrapper(&Curve::get_y_values_as_array),  //
            "set_color", thread_call_wrapper(&Curve::set_color),                           //
            "user_pick_x_coord",
#if 0
															 thread_call_wrapper(&Curve::pick_x_coord) //
#else
            [&script_engine](const Lua_UI_Wrapper<Curve> &lua_curve) {
				abort_check();
				QThread *thread = QThread::currentThread();
                std::promise<double> x_selection_promise;
                std::future<double> x_selection_future = x_selection_promise.get_future();
                Utility::thread_call(MainWindow::mw, [&lua_curve, thread, x_selection_promise = &x_selection_promise ]() mutable {
                    Curve &curve = MainWindow::mw->get_lua_UI_class<Curve>(lua_curve.id);
                    curve.set_onetime_click_callback([thread, x_selection_promise](double x, double y) mutable {
                        (void)y;
                        x_selection_promise->set_value(x);
                        Utility::thread_call(thread, [thread] { thread->exit(1234); });
                    });
                },
                                     &script_engine);
                if (QEventLoop{}.exec() == 1234) {
                    double result = x_selection_future.get();
                    return result;
                } else {
                    throw sol::error("aborted");
                }
            }
//
#endif
            );
        ui_table.new_usertype<Lua_UI_Wrapper<Plot>>(
            "Plot", //

            sol::meta_function::construct, sol::factories([ parent = script_engine.parent, &script_engine ] {
				abort_check();
				return Lua_UI_Wrapper<Plot>{parent, &script_engine};
            }), //
            "clear",
            thread_call_wrapper(&Plot::clear), //
            "add_curve", [ parent = script_engine.parent, &script_engine ](Lua_UI_Wrapper<Plot> & lua_plot)->Lua_UI_Wrapper<Curve> {
                return Utility::promised_thread_call(MainWindow::mw,
                                                     [parent, &lua_plot, &script_engine] {
														 abort_check();
														 auto &plot = MainWindow::mw->get_lua_UI_class<Plot>(lua_plot.id);
                                                         return Lua_UI_Wrapper<Curve>{parent, &script_engine, &script_engine, &plot};
                                                     } //
                                                     );
            }, //
            "set_x_marker",
            thread_call_wrapper(&Plot::set_x_marker), //
            "set_visible", thread_call_wrapper(&Plot::set_visible));
    }
    //bind color
    {
        ui_table.new_usertype<Color>("Color", //
                                     sol::meta_function::construct, sol::no_constructor);
		ui_table["Color"] = overloaded_function(
			[](const std::string &name) {
				abort_check();
				return Color{name};
			},
			[](int r, int g, int b) {
				abort_check();
				return Color{r, g, b};
			}, //
			[](int rgb) {
				abort_check();
				return Color{rgb};
			});
    }
    //bind PollDataEngine
    {
        ui_table.new_usertype<Lua_UI_Wrapper<PollDataEngine>>(
            "PollDataEngine", //
            sol::meta_function::construct,
            sol::factories([ parent = script_engine.parent, &script_engine ](Data_engine_handle & handle, const sol::table items) {
				abort_check();
				QStringList sl;
                for (const auto &item : items) {
                    sl.append(QString::fromStdString(item.second.as<std::string>()));
                }
                return Lua_UI_Wrapper<PollDataEngine>{parent, &script_engine, &script_engine, handle.data_engine, sl};
            }), //

            "set_visible",
            thread_call_wrapper(&PollDataEngine::set_visible),                //
            "refresh", thread_call_wrapper(&PollDataEngine::refresh),         //
            "set_enabled", thread_call_wrapper(&PollDataEngine::set_enabled), //
            "is_in_range", thread_call_wrapper(&PollDataEngine::is_in_range));
    }
    //bind DataEngineInput
    {
        ui_table.new_usertype<Lua_UI_Wrapper<DataEngineInput>>(
            "DataEngineInput", //
            sol::meta_function::construct,
            sol::factories([ parent = script_engine.parent, &script_engine ](Data_engine_handle & handle, const std::string &field_id,
                                                                             const std::string &extra_explanation, const std::string &empty_value_placeholder,
                                                                             const std::string &actual_prefix, const std::string &desired_prefix) {
				abort_check();
				return Lua_UI_Wrapper<DataEngineInput>{parent,        &script_engine,    &script_engine,          handle.data_engine,
                                                       field_id,      extra_explanation, empty_value_placeholder, actual_prefix,
                                                       desired_prefix};
            }), //
            "load_actual_value",
            thread_call_wrapper(&DataEngineInput::load_actual_value),                          //
            "await_event", non_gui_call_wrapper(&DataEngineInput::await_event),                //
            "set_visible", thread_call_wrapper(&DataEngineInput::set_visible),                 //
            "set_enabled", thread_call_wrapper(&DataEngineInput::set_enabled),                 //
            "save_to_data_engine", thread_call_wrapper(&DataEngineInput::save_to_data_engine), //
            "set_editable", thread_call_wrapper(&DataEngineInput::set_editable),               //
            "sleep_ms", non_gui_call_wrapper(&DataEngineInput::sleep_ms),                      //
            "is_editable", thread_call_wrapper(&DataEngineInput::get_is_editable),             //
            "set_explanation_text", thread_call_wrapper(&DataEngineInput::set_explanation_text));
    }
    //bind UserInstructionLabel
    {
        ui_table.new_usertype<Lua_UI_Wrapper<UserInstructionLabel>>(
            "UserInstructionLabel", //
            sol::meta_function::construct, sol::factories([ parent = script_engine.parent, &script_engine ](const std::string &instruction_text) {
				abort_check();
				return Lua_UI_Wrapper<UserInstructionLabel>{parent, &script_engine, &script_engine, instruction_text};
            }), //
            "await_event",
            non_gui_call_wrapper(&UserInstructionLabel::await_event),                  //
            "await_yes_no", non_gui_call_wrapper(&UserInstructionLabel::await_yes_no), //

            "set_visible", thread_call_wrapper(&UserInstructionLabel::set_visible), //
            "set_enabled", thread_call_wrapper(&UserInstructionLabel::set_enabled), //
            "set_instruction_text", thread_call_wrapper(&UserInstructionLabel::set_instruction_text));
    }
    //bind UserWaitLabel
    {
        ui_table.new_usertype<Lua_UI_Wrapper<UserWaitLabel>>(
            "UserWaitLabel", //
            sol::meta_function::construct, sol::factories([ parent = script_engine.parent, &script_engine ](const std::string &instruction_text) {
				abort_check();
				return Lua_UI_Wrapper<UserWaitLabel>{parent, &script_engine, &script_engine, instruction_text};
            }), //
            "set_enabled",
            thread_call_wrapper(&UserWaitLabel::set_enabled), //
            "set_text", thread_call_wrapper(&UserWaitLabel::set_text));
    }
    //bind ComboBoxFileSelector
    {
        ui_table.new_usertype<Lua_UI_Wrapper<ComboBoxFileSelector>>(
            "ComboBoxFileSelector", //
            sol::meta_function::construct,
            sol::factories([ parent = script_engine.parent, path = path, &script_engine ](const std::string &directory, const sol::table &filters) {
				abort_check();
				QStringList sl;
                for (const auto &item : filters) {
                    sl.append(QString::fromStdString(item.second.as<std::string>()));
                }
                return Lua_UI_Wrapper<ComboBoxFileSelector>{parent, &script_engine, get_absolute_file_path(QString::fromStdString(path), directory), sl};
            }), //
            "get_selected_file",
            thread_call_wrapper(&ComboBoxFileSelector::get_selected_file),           //
            "set_visible", thread_call_wrapper(&ComboBoxFileSelector::set_visible),  //
            "set_order_by", thread_call_wrapper(&ComboBoxFileSelector::set_order_by) //

            );
    }
    //bind IsotopeSourceSelector
    {
        ui_table.new_usertype<Lua_UI_Wrapper<IsotopeSourceSelector>>("IsotopeSourceSelector", //
                                                                     sol::meta_function::construct,
                                                                     sol::factories([ parent = script_engine.parent, &script_engine ]() {
																		 abort_check();
																		 return Lua_UI_Wrapper<IsotopeSourceSelector>{parent, &script_engine};
                                                                     }), //
                                                                     "set_visible",
                                                                     thread_call_wrapper(&IsotopeSourceSelector::set_visible),                            //
                                                                     "set_enabled", thread_call_wrapper(&IsotopeSourceSelector::set_enabled),             //
                                                                     "filter_by_isotope", thread_call_wrapper(&IsotopeSourceSelector::filter_by_isotope), //
                                                                     "get_selected_activity_Bq",
                                                                     thread_call_wrapper(&IsotopeSourceSelector::get_selected_activity_Bq),               //
                                                                     "get_selected_name", thread_call_wrapper(&IsotopeSourceSelector::get_selected_name), //
                                                                     "get_selected_serial_number",
                                                                     thread_call_wrapper(&IsotopeSourceSelector::get_selected_serial_number) //
                                                                     );
    }
    //bind ComboBox
    {
        ui_table.new_usertype<Lua_UI_Wrapper<ComboBox>>("ComboBox", //
                                                        sol::meta_function::construct,
                                                        sol::factories([ parent = script_engine.parent, &script_engine ](const sol::table &items) {
															abort_check();
															return Lua_UI_Wrapper<ComboBox>{parent, &script_engine, items};
                                                        }), //
                                                        "set_items",
                                                        thread_call_wrapper(&ComboBox::set_items),                   //
                                                        "get_text", thread_call_wrapper(&ComboBox::get_text),        //
                                                        "set_index", thread_call_wrapper(&ComboBox::set_index),      //
                                                        "get_index", thread_call_wrapper(&ComboBox::get_index),      //
                                                        "set_visible", thread_call_wrapper(&ComboBox::set_visible),  //
                                                        "set_name", thread_call_wrapper(&ComboBox::set_name),        //
                                                        "set_caption", thread_call_wrapper(&ComboBox::set_caption),  //
                                                        "get_caption", thread_call_wrapper(&ComboBox::get_caption),  //
                                                        "set_editable", thread_call_wrapper(&ComboBox::set_editable) //
                                                        );
    }
    //bind SpinBox
    {
        ui_table.new_usertype<Lua_UI_Wrapper<SpinBox>>("SpinBox", //
                                                       sol::meta_function::construct, sol::factories([ parent = script_engine.parent, &script_engine ]() {
														   abort_check();
														   return Lua_UI_Wrapper<SpinBox>{parent, &script_engine}; //
                                                       }),                                                         //
                                                       "get_value",
                                                       thread_call_wrapper(&SpinBox::get_value),                      //
                                                       "set_max_value", thread_call_wrapper(&SpinBox::set_max_value), //
                                                       "set_min_value", thread_call_wrapper(&SpinBox::set_min_value), //
                                                       "set_value", thread_call_wrapper(&SpinBox::set_value),         //
                                                       "set_visible", thread_call_wrapper(&SpinBox::set_visible),     //
                                                       "set_caption", thread_call_wrapper(&SpinBox::set_caption),     //
                                                       "get_caption", thread_call_wrapper(&SpinBox::get_caption)      //
                                                       );
    }
    //bind ProgressBar
    {
        ui_table.new_usertype<Lua_UI_Wrapper<ProgressBar>>("ProgressBar", //
                                                           sol::meta_function::construct, sol::factories([ parent = script_engine.parent, &script_engine ]() {
															   abort_check();
															   return Lua_UI_Wrapper<ProgressBar>{parent, &script_engine};
                                                           }), //
                                                           "set_max_value",
                                                           thread_call_wrapper(&ProgressBar::set_max_value),                      //
                                                           "set_min_value", thread_call_wrapper(&ProgressBar::set_min_value),     //
                                                           "set_value", thread_call_wrapper(&ProgressBar::set_value),             //
                                                           "increment_value", thread_call_wrapper(&ProgressBar::increment_value), //
                                                           "set_visible", thread_call_wrapper(&ProgressBar::set_visible),         //
                                                           "set_caption", thread_call_wrapper(&ProgressBar::set_caption),         //
                                                           "get_caption", thread_call_wrapper(&ProgressBar::get_caption)          //
                                                           );
    }
    //bind Label
    {
        ui_table.new_usertype<Lua_UI_Wrapper<Label>>("Label", //
                                                     sol::meta_function::construct,
                                                     sol::factories([ parent = script_engine.parent, &script_engine ](const std::string &text) {
														 abort_check();
														 return Lua_UI_Wrapper<Label>{parent, &script_engine, text};
                                                     }), //
                                                     "set_text",
                                                     thread_call_wrapper(&Label::set_text),                       //
                                                     "set_enabled", thread_call_wrapper(&Label::set_enabled),     //
                                                     "set_visible", thread_call_wrapper(&Label::set_visible),     //
                                                     "set_font_size", thread_call_wrapper(&Label::set_font_size), //
                                                     "get_text", thread_call_wrapper(&Label::get_text));
    }
    //bind hline
    {
        ui_table.new_usertype<Lua_UI_Wrapper<HLine>>("HLine", //
                                                     sol::meta_function::construct, sol::factories([ parent = script_engine.parent, &script_engine ]() {
														 abort_check();
														 return Lua_UI_Wrapper<HLine>{parent, &script_engine};
                                                     }), //

                                                     "set_visible",
                                                     thread_call_wrapper(&HLine::set_visible));
    }
    //bind CheckBox
    {
        ui_table.new_usertype<Lua_UI_Wrapper<CheckBox>>("CheckBox", //
                                                        sol::meta_function::construct,
                                                        sol::factories([ parent = script_engine.parent, &script_engine ](const std::string &text) {
															abort_check();
															return Lua_UI_Wrapper<CheckBox>{parent, &script_engine, text};
                                                        }), //
                                                        "set_checked",
                                                        thread_call_wrapper(&CheckBox::set_checked),                //
                                                        "get_checked", thread_call_wrapper(&CheckBox::get_checked), //
                                                        "set_visible", thread_call_wrapper(&CheckBox::set_visible), //
                                                        "set_text", thread_call_wrapper(&CheckBox::set_text),       //
                                                        "get_text", thread_call_wrapper(&CheckBox::get_text));
    }
    //bind Image

    {
        ui_table.new_usertype<Lua_UI_Wrapper<Image>>("Image", sol::meta_function::construct,
                                                     sol::factories([ parent = script_engine.parent, path = path, &script_engine ]() {
														 abort_check();
														 return Lua_UI_Wrapper<Image>{parent, &script_engine, QString::fromStdString(path)}; //
                                                     }),
                                                     "load_image_file", thread_call_wrapper(&Image::load_image_file), //
                                                     "set_visible", thread_call_wrapper(&Image::set_visible)          //
                                                     );
    }
    //bind button
    {
        ui_table.new_usertype<Lua_UI_Wrapper<Button>>("Button", //
                                                      sol::meta_function::construct,
                                                      sol::factories([ parent = script_engine.parent, &script_engine ](const std::string &title) {
														  abort_check();
														  return Lua_UI_Wrapper<Button>{parent, &script_engine, &script_engine, title};
                                                      }), //
                                                      "has_been_clicked",
                                                      thread_call_wrapper(&Button::has_been_clicked),                       //
                                                      "set_visible", thread_call_wrapper(&Button::set_visible),             //
                                                      "reset_click_state", thread_call_wrapper(&Button::reset_click_state), //
                                                      "await_click", non_gui_call_wrapper(&Button::await_click)             //
                                                      );
    }

    //bind edit field
    {
        ui_table.new_usertype<Lua_UI_Wrapper<LineEdit>>(
            "LineEdit", //
            sol::meta_function::construct,
            sol::factories([ parent = script_engine.parent, &script_engine ] { return Lua_UI_Wrapper<LineEdit>(parent, &script_engine, &script_engine); }), //
            "set_placeholder_text", thread_call_wrapper(&LineEdit::set_placeholder_text),                                                                   //
            "get_text", thread_call_wrapper(&LineEdit::get_text),                                                                                           //
            "set_text", thread_call_wrapper(&LineEdit::set_text),                                                                                           //
            "set_name", thread_call_wrapper(&LineEdit::set_name),                                                                                           //
            "get_name", thread_call_wrapper(&LineEdit::get_name),                                                                                           //
            "get_number", thread_call_wrapper(&LineEdit::get_number),                                                                                       //
            "get_caption", thread_call_wrapper(&LineEdit::get_caption),                                                                                     //
            "set_caption", thread_call_wrapper(&LineEdit::set_caption),                                                                                     //
            "set_enabled", thread_call_wrapper(&LineEdit::set_enabled),                                                                                     //
            "set_visible", thread_call_wrapper(&LineEdit::set_visible),                                                                                     //
            "set_focus", thread_call_wrapper(&LineEdit::set_focus),                                                                                         //
            "await_return", non_gui_call_wrapper(&LineEdit::await_return),                                                                                  //
            "get_date", thread_call_wrapper(&LineEdit::get_date),                                                                                           //
            "set_date", thread_call_wrapper(&LineEdit::set_date),                                                                                           //
            "set_date_mode", thread_call_wrapper(&LineEdit::set_date_mode),                                                                                 //
            "set_text_mode", thread_call_wrapper(&LineEdit::set_text_mode),                                                                                 //
            "set_input_check", thread_call_wrapper(&LineEdit::set_input_check),                                                                             //

            "load_from_cache", non_gui_call_wrapper(&LineEdit::load_from_cache) //
            );
    }
    {
		lua.new_usertype<SCPIDevice>("SCPIDevice",                                       //
									 sol::meta_function::construct, sol::no_constructor, //
									 "get_protocol_name",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_protocol_name();
									 }, //
									 "get_device_descriptor",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_device_descriptor();
									 }, //
									 "get_str",
									 [](SCPIDevice &protocol, std::string request) {
										 abort_check();
										 return protocol.get_str(request);
									 }, //
									 "get_str_param",
									 [](SCPIDevice &protocol, std::string request, std::string argument) {
										 abort_check();
										 return protocol.get_str_param(request, argument);
									 }, //
									 "get_num",
									 [](SCPIDevice &protocol, std::string request) {
										 abort_check();
										 return protocol.get_num(request);
									 }, //
									 "get_num_param",
									 [](SCPIDevice &protocol, std::string request, std::string argument) {
										 abort_check();
										 return protocol.get_num_param(request, argument);
									 }, //
									 "get_name",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_name();
									 }, //
									 "get_serial_number",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_serial_number();
									 }, //
									 "get_manufacturer",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_manufacturer();
									 }, //
									 "get_calibration",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_calibration();
									 }, //
									 "is_event_received",
									 [](SCPIDevice &protocol, std::string event_name) {
										 abort_check();
										 return protocol.is_event_received(event_name);
									 }, //
									 "clear_event_list",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.clear_event_list();
									 }, //
									 "get_event_list",
									 [](SCPIDevice &protocol) {
										 abort_check();
										 return protocol.get_event_list();
									 }, //
									 "set_validation_max_standard_deviation",
									 [](SCPIDevice &protocoll, double max_std_dev) {
										 abort_check();
										 return protocoll.set_validation_max_standard_deviation(max_std_dev);
									 }, //
									 "set_validation_retries",
									 [](SCPIDevice &protocoll, unsigned int retries) {
										 abort_check();
										 return protocoll.set_validation_retries(retries);
									 }, //
									 "send_command",
									 [](SCPIDevice &protocoll, std::string request) {
										 abort_check();
										 return protocoll.send_command(request);
									 } //

									 );
    }

    {
		lua.new_usertype<SG04CountDevice>("SG04CountDevice",                                  //
										  sol::meta_function::construct, sol::no_constructor, //
										  "get_protocol_name",
										  [](SG04CountDevice &protocol) {
											  abort_check();
											  return protocol.get_protocol_name();
										  }, //
                                          "get_name",
										  [](SG04CountDevice &protocol) {
											  abort_check();
											  (void)protocol;
                                              return "SG04";
                                          }, //
                                          "get_sg04_counts",
										  [](SG04CountDevice &protocol, bool clear_on_read) {
											  abort_check();
											  return protocol.get_sg04_counts(clear_on_read);
										  }, //
										  "accumulate_counts",
										  [](SG04CountDevice &protocol, uint time_ms) {
											  abort_check();
											  return protocol.accumulate_counts(time_ms);
										  } //
                                          );
    }
    {
		lua.new_usertype<ManualDevice>("ManualDevice",                                     //
									   sol::meta_function::construct, sol::no_constructor, //
									   "get_protocol_name",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_protocol_name();
									   }, //
									   "get_name",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_name();
									   }, //
									   "get_manufacturer",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_manufacturer();
									   }, //
									   "get_description",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_description();
									   }, //
									   "get_serial_number",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_serial_number();
									   }, //
									   "get_notes",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_notes();
									   }, //
									   "get_calibration",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_calibration();
									   }, //

									   "get_summary",
									   [](ManualDevice &protocol) {
										   abort_check();
										   return protocol.get_summary();
									   } //

                                       );
    }
}
