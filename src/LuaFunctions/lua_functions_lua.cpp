#include "lua_functions_lua.h"
#include "Windows/devicematcher.h"
#include "config.h"
#include "lua_functions.h"
#include "scriptsetup_helper.h"
#include "ui_container.h"

#include <QDir>
#include <QSettings>

void bind_lua_functions(sol::state &lua, sol::table &ui_table, const std::string &path, ScriptEngine &script_engine, QPlainTextEdit *console) {
    //General functions
    {
        lua["show_file_save_dialog"] = [path](const std::string &title, const std::string &preselected_path, sol::table filters) {
            abort_check();
            return show_file_save_dialog(title, get_absolute_file_path(QString::fromStdString(path), preselected_path), filters);
        };
        lua["show_file_open_dialog"] = [path](const std::string &title, const std::string &preselected_path, sol::table filters) {
            abort_check();
            return show_file_open_dialog(title, get_absolute_file_path(QString::fromStdString(path), preselected_path), filters);
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

        lua["print"] = [console = console](const sol::variadic_args &args) {
            abort_check();
            print(console, args);
        };

        lua["sleep_ms"] = [&script_engine](const unsigned int duration_ms) {
            //either sleep_ms(duration) or sleep_ms(start, duration), so can't name parameters properly
            abort_check();
            sleep_ms(&script_engine, duration_ms, 0);
        };
        lua["pc_speaker_beep"] = wrap(pc_speaker_beep);
        lua["current_date_time_ms"] = wrap(current_date_time_ms);
        lua["round"] = +[](const double value, const unsigned int precision = 0) {
            abort_check();
            return round_double(value, precision);
        };
    }
    //table functions
    {
        lua["table_save_to_file"] = [console = console, path = path](const std::string file_name, sol::table input_table, bool over_write_file) {
            abort_check();
            table_save_to_file(console, get_absolute_file_path(QString::fromStdString(path), file_name), input_table, over_write_file);
        };
        lua["table_load_from_file"] = [&lua, console = console, path = path](const std::string file_name) {
            abort_check();
            return table_load_from_file(console, lua, get_absolute_file_path(QString::fromStdString(path), file_name));
        };
        lua["table_sum"] = wrap(&table_sum);
        lua["table_find_string"] = wrap(&table_find_string);
        lua["table_contains_string"] = wrap(&table_contains_string);
        lua["table_crc16"] = [console = console](sol::table table) {
            abort_check();
            return table_crc16(console, table);
        };

        lua["table_mean"] = wrap(&table_mean);
        lua["table_variance"] = wrap(&table_variance);
        lua["table_standard_deviation"] = wrap(&table_standard_deviation);
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

        lua["table_range"] = [&lua](double start, double stop, double step) {
            abort_check();
            return table_range(lua, start, stop, step);
        };
        lua["table_concat"] = [&lua](sol::table table1, sol::table table2) {
            abort_check();
            return table_concat(lua, table1, table2);
        };

        lua["table_equal_table"] = [&lua](sol::table table1, sol::table table2) {
            abort_check();
            return table_equal_table(lua, table1, table2);
        };
        lua["table_equal_constant"] = wrap(&table_equal_constant);
        lua["table_max"] = wrap(&table_max);
        lua["table_min"] = wrap(&table_min);
        lua["table_max_abs"] = wrap(&table_max_abs);
        lua["table_min_abs"] = wrap(&table_min_abs);
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
        lua["git_info"] = [&lua, path = path](std::string dir_path, bool allow_modified) {
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

        lua["get_framework_git_hash"] = wrap(get_framework_git_hash);
        lua["get_framework_git_date_unix"] = wrap(get_framework_git_date_unix);
        lua["get_framework_git_date_text"] = wrap(get_framework_git_date_text);
        lua["get_os_username"] = wrap(get_os_username);
        lua["mk_link"] = wrap(mk_link);
        lua["file_link_points_to"] = wrap(file_link_points_to);
        lua["is_file_path_equal"] = wrap(is_file_path_equal);
        lua["path_exists"] = wrap(path_exists);
    }
    //noise level
    {
        lua["measure_noise_level_czt"] = [&lua](sol::table rpc_device, const unsigned int dacs_quantity, const unsigned int max_possible_dac_value) {
            abort_check();
            return measure_noise_level_czt(lua, rpc_device, dacs_quantity, max_possible_dac_value);
        };
    }
    //Add device discovery functions
    {
        lua["discover_devices"] = [&script_engine](const sol::table &device_description) {
            abort_check();
            if (device_description.empty()) {
                return device_description; //empty device description, return empty matches
            }
            auto devices = MainWindow::mw->discover_devices(script_engine, device_description);
            while (devices.empty()) {
                abort_check();
                Utility::promised_thread_call(
                    MainWindow::mw, +[] {
                        MainWindow::mw->on_actionrefresh_devices_all_triggered(); //does not wait for devices to be refreshed, so we wait afterwards
                    });
                std::this_thread::sleep_for(std::chrono::seconds(2));
                devices = MainWindow::mw->discover_devices(script_engine, device_description);
            }
            return script_engine.get_devices(devices);
        };
        lua.safe_script(R"(
			function discover_device(device)
				return discover_devices({device})
			end
		)");
        lua["refresh_devices"] = +[] {
            abort_check();
            Utility::promised_thread_call(
                MainWindow::mw, +[] {
                    MainWindow::mw->on_actionrefresh_devices_all_triggered(); //does not wait for devices to be refreshed, so we wait afterwards
                });
            std::this_thread::sleep_for(std::chrono::seconds(2));
        };
        lua["refresh_DUTs"] = +[] {
            abort_check();
            Utility::promised_thread_call(
                MainWindow::mw, +[] {
                    MainWindow::mw->on_actionrefresh_devices_dut_triggered(); //does not wait for devices to be refreshed, so we wait afterwards
                });
            std::this_thread::sleep_for(std::chrono::seconds(2));
        };
    }
    //UI functions
    {
        ui_table["set_column_count"] = [container = script_engine.parent, &script_engine](int count) {
            abort_check();
            Utility::thread_call(
                MainWindow::mw, [container, count] { container->set_column_count(count); }, &script_engine);
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
    //bind charge UserEntryCache
    {
#if 0
		lua.new_usertype<UserEntryCache>("UserEntryCache", //
										  sol::meta_function::construct,sol::factories(
										  +[] { //
											  abort_check();
											  return UserEntryCache{};
										  }));
#endif
    }
    //set up import functionality
    {
        lua["find_script"] = [&script_engine](const std::string &name) { return script_engine.get_script_import_path(name); };
        lua.safe_script(R"xx(
				   assert(_VERSION == "Lua 5.3")
				   --might also work with 5.2
				   --definitely does not work with 5.1 and below because _ENV did not exist
				   --may work with 5.4+, but that does not exist at time of writing

				   function env_copy(obj)
					   obj = obj or _ENV
					   local res = {}
					   for k, v in pairs(obj) do
                           if string.find("boolean number string function table", type(v)) then --not copying "thread" and "userdata"
                               --print( "Copied " .. k .. " of type " .. type(v))
                               res[k] = v
                           else
                               --print( "Skipped " .. k .. " of type " .. type(v))
                           end
					   end
					   return res
				   end

				   local default_env = env_copy()

				   function import(name)
					   local env = env_copy(default_env)
					   assert(loadfile(assert(find_script(name)), name, env))()
					   return env
				   end
				)xx",
                        "import function");
    }
    //set up require paths
#if 0
    {
        lua.safe_script("package.path = \"" + QSettings{}.value(Globals::test_script_path_settings_key, "").toString().replace("\\", "\\\\").toStdString() +
                        "/?.lua\"");
    }
#endif
    //set up require paths
    {
        lua.safe_script("package.path = \"" + get_lua_lib_search_paths_for_lua("", "/?.lua").join(";").toStdString() + std::string("\""));
    } //add generic function
    {
//TODO: Figure out why the lambda version crashes on Windows and the Require version does not.
#if 1
        struct Require {
            std::string path;
            sol::state &lua;
            sol::protected_function_result operator()(const std::string &file) const {
                auto search_paths = get_lua_lib_search_paths_for_lua(QString::fromStdString(path), "");
                QString tried_paths;
                for (auto &p : search_paths) {
                    QDir dir(p);
                    auto abs_path = dir.absoluteFilePath(QString::fromStdString(file) + ".lua");
                    tried_paths += '\n' + abs_path;
                    if (QFile::exists(abs_path)) {
                        return lua.script_file(abs_path.toStdString());
                    }
                }
                throw std::runtime_error("Can not find a path to required module \"" + file + "\" for script: " + path +
                                         ". Tried paths: " + tried_paths.toStdString());
            }
        };

        lua["require"] = Require{path, lua};
#else

        lua["require"] = [path, &lua](const std::string &file) {
            abort_check();
            QDir dir(QString::fromStdString(path));
            dir.cdUp();
            lua.script_file(dir.absoluteFilePath(QString::fromStdString(file) + ".lua").toStdString());
        };
#endif
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
                default: {
                    throw sol::error("interrupted");
                }
            }
        };
    }
}
