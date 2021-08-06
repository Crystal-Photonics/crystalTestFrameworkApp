#include "dataengineinput_lua.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h"
#include "dataengineinput.h"
#include "lua_functions.h"
#include "scriptsetup_helper.h"

#include <QFileInfo>
#include <QSettings>
#include <fstream>

void bind_dataengineinput(sol::state &lua, sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent, const std::string &path) {
    ui_table.new_usertype<Lua_UI_Wrapper<DataEngineInput>>(
        "DataEngineInput", //
        sol::meta_function::construct,
        sol::factories([parent = parent, &script_engine](Data_engine_handle &handle, const std::string &field_id, const std::string &extra_explanation,
                                                         const std::string &empty_value_placeholder, const std::string &actual_prefix,
                                                         const std::string &desired_prefix) {
            abort_check();
            return Lua_UI_Wrapper<DataEngineInput>{parent,        &script_engine,    &script_engine,          handle.data_engine.get(),
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
    //bind data engine
    {
        lua.new_usertype<Data_engine_handle>(
            "Data_engine", //
            sol::meta_function::construct,
            sol::factories([path = path, &script_engine](const std::string &pdf_template_file, const std::string &json_file,
                                                         const std::string &auto_json_dump_path, const sol::table &dependency_tags) {
                abort_check();
                Data_engine_handle de{std::make_unique<Data_engine>(), script_engine.console};
                de.data_engine->set_script_path(script_engine.path_m);
                de.data_engine->enable_logging(script_engine.console, *script_engine.matched_devices);
                auto file_path = get_absolute_file_path(QString::fromStdString(path), json_file);
                auto xml_file = get_absolute_file_path(QString::fromStdString(path), pdf_template_file);
                auto auto_dump_path = get_absolute_file_path(QString::fromStdString(path), auto_json_dump_path);
                std::ifstream f(file_path);
                if (!f) {
                    throw std::runtime_error("Failed opening file " + file_path);
                }

                auto add_value_to_tag_list = +[](QList<QVariant> &values, const sol::object &obj, const std::string &tag_name) {
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
                de.data_engine->set_dependancy_tags(tags);
                de.data_engine->set_source(f);
                de.data_engine->source_path = QString::fromStdString(file_path);
                de.data_engine_pdf_template_path = QString::fromStdString(xml_file);
                de.data_engine_auto_dump_path = QString::fromStdString(auto_dump_path);

                return de;
            }), //
            "set_start_time_seconds_since_epoch",
            +[](Data_engine_handle &handle, const double start_time) {
                abort_check();
                return handle.data_engine->set_start_time_seconds_since_epoch(start_time);
            },
            "use_instance",
            +[](Data_engine_handle &handle, const std::string &section_name, const std::string &instance_caption, const uint instance_index) {
                abort_check();
                handle.data_engine->use_instance(QString::fromStdString(section_name), QString::fromStdString(instance_caption), instance_index);
            },
            "start_recording_actual_value_statistic",
            +[](Data_engine_handle &handle, const std::string &root_file_path, const std::string &prefix) {
                abort_check();
                return handle.data_engine->start_recording_actual_value_statistic(root_file_path, prefix);
            },
            "set_dut_identifier",
            +[](Data_engine_handle &handle, const std::string &dut_identifier) {
                abort_check();
                return handle.data_engine->set_dut_identifier(QString::fromStdString(dut_identifier));
            },

            "get_instance_count",
            +[](Data_engine_handle &handle, const std::string &section_name) {
                abort_check();
                return handle.data_engine->get_instance_count(section_name);
            },

            "get_description",
            +[](Data_engine_handle &handle, const std::string &id) {
                abort_check();
                return handle.data_engine->get_description(QString::fromStdString(id)).toStdString();
            },
            "get_actual_value",
            +[](Data_engine_handle &handle, const std::string &id) {
                abort_check();
                return handle.data_engine->get_actual_value(QString::fromStdString(id)).toStdString();
            },
            "get_actual_number",
            +[](Data_engine_handle &handle, const std::string &id) {
                abort_check();
                return handle.data_engine->get_actual_number(QString::fromStdString(id));
            },

            "get_unit",
            +[](Data_engine_handle &handle, const std::string &id) {
                abort_check();
                return handle.data_engine->get_unit(QString::fromStdString(id)).toStdString();
            },
            "get_desired_value",
            +[](Data_engine_handle &handle, const std::string &id) {
                abort_check();
                return handle.data_engine->get_desired_value_as_string(QString::fromStdString(id)).toStdString();
            },
            "get_desired_number",
            +[](Data_engine_handle &handle, const std::string &id) {
                abort_check();
                return handle.data_engine->get_desired_number(QString::fromStdString(id));
            },
            "get_section_names",
            [&lua](Data_engine_handle &handle) {
                abort_check();
                return handle.data_engine->get_section_names(&lua).table;
            }, //
            "get_ids_of_section",
            [&lua](Data_engine_handle &handle, const std::string &section_name) {
                abort_check();
                return handle.data_engine->get_ids_of_section(&lua, section_name).table;
            },
            "set_instance_count",
            +[](Data_engine_handle &handle, const std::string &instance_count_name, const uint instance_count) {
                abort_check();
                handle.data_engine->set_instance_count(QString::fromStdString(instance_count_name), instance_count);
            },
            "save_to_json",
            [path = path](Data_engine_handle &handle, const std::string &file_name) {
                abort_check();
                auto fn = get_absolute_file_path(QString::fromStdString(path), file_name);
                handle.data_engine->save_to_json(QString::fromStdString(fn));
            },
            "add_extra_pdf_path",
            [path = path](Data_engine_handle &handle, const std::string &file_name) {
                abort_check();
                (void)handle;
                // data_engine_auto_dump_path = QString::fromStdString(auto_dump_path);
                handle.additional_pdf_path = QString::fromStdString(get_absolute_file_path(QString::fromStdString(path), file_name));
                //  handle.data_engine->save_to_json(QString::fromStdString(fn));
            },
            "set_open_pdf_on_pdf_creation",
            [path = path](Data_engine_handle &handle, bool auto_open_on_pdf_creation) {
                abort_check();
                handle.data_engine->set_enable_auto_open_pdf(auto_open_on_pdf_creation);
            },
            "value_in_range",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->value_in_range(QString::fromStdString(field_id));
            },
            "value_in_range_in_section",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->value_in_range_in_section(QString::fromStdString(field_id));
            },
            "value_in_range_in_instance",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->value_in_range_in_instance(QString::fromStdString(field_id));
            },
            "values_in_range",
            +[](Data_engine_handle &handle, const sol::table &field_ids) {
                abort_check();
                QList<FormID> ids;
                for (const auto &field_id : field_ids) {
                    ids.append(QString::fromStdString(field_id.second.as<std::string>()));
                }
                return handle.data_engine->values_in_range(ids);
            },
            "is_bool",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->is_bool(QString::fromStdString(field_id));
            },
            "is_text",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->is_text(QString::fromStdString(field_id));
            },
            "is_datetime",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->is_datetime(QString::fromStdString(field_id));
            },
            "is_number",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->is_number(QString::fromStdString(field_id));
            },
            "is_exceptionally_approved",
            +[](Data_engine_handle &handle, const std::string &field_id) {
                abort_check();
                return handle.data_engine->is_exceptionally_approved(QString::fromStdString(field_id));
            },
            "set_actual_number",
            +[](Data_engine_handle &handle, const std::string &field_id, double value) {
                abort_check();
                handle.data_engine->set_actual_number(QString::fromStdString(field_id), value);
            },
            "set_actual_bool",
            +[](Data_engine_handle &handle, const std::string &field_id, bool value) {
                abort_check();
                handle.data_engine->set_actual_bool(QString::fromStdString(field_id), value);
            },
#if 1
            "set_actual_datetime",
            +[](Data_engine_handle &handle, const std::string &field_id, double value) {
                abort_check();
                handle.data_engine->set_actual_datetime(QString::fromStdString(field_id), DataEngineDateTime{value});
            },
#endif
#if 1
            "set_actual_datetime_from_text",
            +[](Data_engine_handle &handle, const std::string &field_id, const std::string &text) {
                abort_check();
                handle.data_engine->set_actual_datetime(QString::fromStdString(field_id), DataEngineDateTime{QString::fromStdString(text)});
            },
#endif
            "set_actual_text",
            +[](Data_engine_handle &handle, const std::string &field_id, const std::string &text) {
                abort_check();
                handle.data_engine->set_actual_text(QString::fromStdString(field_id), QString::fromStdString(text));
            },
            "all_values_in_range", +[](Data_engine_handle &handle) { return handle.data_engine->all_values_in_range(); });
    }
}
///\cond HIDDEN_SYMBOLS
Data_engine_handle::~Data_engine_handle() {
    if (not data_engine) { //moved from
        return;
    }
    ExceptionalApprovalDB ea_db{QSettings{}.value(Globals::path_to_excpetional_approval_db_key, "").toString()};
    try {
        data_engine->do_exceptional_approvals(ea_db, MainWindow::mw);
        data_engine->save_actual_value_statistic();
        if (data_engine_pdf_template_path.count() and data_engine_auto_dump_path.count()) {
            QFileInfo fi(data_engine_auto_dump_path);
            QString suffix = fi.completeSuffix();
            if (suffix == "") {
                QString path = append_separator_to_path(fi.absoluteFilePath());
                path += "report.pdf";
                fi.setFile(path);
            }
            //fi.baseName("/home/abc/report.pdf") = "report"
            //fi.absolutePath("/home/abc/report.pdf") = "/home/abc/"

            std::string target_filename = propose_unique_filename_by_datetime(fi.absolutePath().toStdString(), fi.baseName().toStdString(), ".pdf");
            target_filename.resize(target_filename.size() - 4);

            try {
                data_engine->generate_pdf(data_engine_pdf_template_path.toStdString(), target_filename + ".pdf");
                data_engine->set_log_file(target_filename + "_log.txt");
                if (additional_pdf_path.count()) {
                    QFile::copy(QString::fromStdString(target_filename), additional_pdf_path);
                }
            } catch (const DataEngineError &dee) {
                Utility::promised_thread_call(MainWindow::mw, [this, &dee] { console->error() << dee.what(); });
            }
        }
        if (data_engine_auto_dump_path.count()) {
            QFileInfo fi(data_engine_auto_dump_path);
            QString suffix = fi.completeSuffix();
            if (suffix == "") {
                QString path = append_separator_to_path(fi.absoluteFilePath());
                path += "dump.json";
                fi.setFile(path);
            }
            std::string json_target_filename = propose_unique_filename_by_datetime(fi.absolutePath().toStdString(), fi.baseName().toStdString(), ".json");

            data_engine->save_to_json(QString::fromStdString(json_target_filename));
        }
    } catch (const DataEngineError &e) {
        console->error() << e.what();
    } catch (const std::runtime_error &e) {
        console->error() << e.what();
    }
}
/// \endcond
