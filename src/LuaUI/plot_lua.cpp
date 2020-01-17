#include "plot_lua.h"
#include "plot.h"
#include "scriptsetup_helper.h"

void bind_plot(sol::table &ui_table, ScriptEngine &script_engine, UI_container *parent) {
    ui_table.new_usertype<Lua_UI_Wrapper<Curve>>(
        "Curve",                                                                               //
        sol::meta_function::construct, sol::no_constructor,                                    //
        "append_point", thread_call_wrapper_non_waiting(&script_engine, &Curve::append_point), //
        "append", thread_call_wrapper_non_waiting(&script_engine, &Curve::append),             //
        "add_spectrum",
        [&script_engine](Lua_UI_Wrapper<Curve> &curve, sol::table table) {
            abort_check();
            std::vector<double> data;
            data.reserve(table.size());
            for (auto &i : table) {
                data.push_back(i.second.as<double>());
            }
            Utility::thread_call(
                MainWindow::mw,
                [id = curve.id, data = std::move(data)] {
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
            Utility::thread_call(
                MainWindow::mw,
                [id = curve.id, data = std::move(data), spectrum_start_channel] {
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
        "pick_x_coord", non_gui_call_wrapper(&Curve::pick_x_coord)                     //
    );
    ui_table.new_usertype<Lua_UI_Wrapper<Plot>>(
        "Plot", //

        sol::meta_function::construct, sol::factories([parent = parent, &script_engine] {
            abort_check();
            return Lua_UI_Wrapper<Plot>{parent, &script_engine};
        }), //
        "clear",
        thread_call_wrapper(&Plot::clear), //
        "add_curve",
        [parent = parent, &script_engine](Lua_UI_Wrapper<Plot> &lua_plot) -> Lua_UI_Wrapper<Curve> {
            return Utility::promised_thread_call(MainWindow::mw,
                                                 [parent, &lua_plot, &script_engine] {
                                                     abort_check();
                                                     auto &plot = MainWindow::mw->get_lua_UI_class<Plot>(lua_plot.id);
                                                     return Lua_UI_Wrapper<Curve>{parent, &script_engine, &script_engine, &plot};
                                                 } //
            );
        }, //
        "set_x_marker",
        thread_call_wrapper(&Plot::set_x_marker),                    //
        "set_visible", thread_call_wrapper(&Plot::set_visible),      //
        "set_time_scale", thread_call_wrapper(&Plot::set_time_scale) //
    );
}
