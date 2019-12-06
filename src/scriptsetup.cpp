#include "scriptsetup.h"
#include "LuaUI/button_lua.h"
#include "LuaUI/checkbox_lua.h"
#include "LuaUI/color_lua.h"
#include "LuaUI/combobox_lua.h"
#include "LuaUI/combofileselector_lua.h"
#include "LuaUI/dataengineinput_lua.h"
#include "LuaUI/hline_lua.h"
#include "LuaUI/image_lua.h"
#include "LuaUI/isotopesourceselector_lua.h"
#include "LuaUI/label_lua.h"
#include "LuaUI/lineedit_lua.h"
#include "LuaUI/plot_lua.h"
#include "LuaUI/polldataengine_lua.h"
#include "LuaUI/progressbar_lua.h"
#include "LuaUI/spinbox_lua.h"
#include "LuaUI/userinstructionlabel_lua.h"
#include "LuaUI/userwaitlabel_lua.h"
#include "Protocols/manualprotocol_lua.h"
#include "Protocols/scpiprotocol_lua.h"
#include "Protocols/sg04countprotocol_lua.h"
#include "chargecounter_lua.h"
#include "config.h"
#include "console.h"
#include "data_engine/data_engine.h" //TODO: Figure out why that is required
#include "datalogger_lua.h"
#include "environmentvariables.h"
#include "lua_functions_lua.h"
#include "sol.hpp"

#include <QSettings>
#include <QString>

void script_setup(sol::state &lua, const std::string &path, ScriptEngine &script_engine, UI_container *script_engine_parent,
				  QPlainTextEdit *script_engine_console_plaintext) {
    //load the standard libs
    lua.open_libraries();
    //load environment variables
    EnvironmentVariables env_variables(QSettings{}.value(Globals::path_to_environment_variables_key, "").toString());
    env_variables.load_to_lua(&lua);

    //bind UI
    auto ui_table = lua.create_named_table("Ui");
	bind_button(ui_table, script_engine, script_engine_parent);
	bind_checkbox(ui_table, script_engine, script_engine_parent);
	bind_color(ui_table, script_engine, script_engine_parent);
	bind_combobox(ui_table, script_engine, script_engine_parent);
	bind_combofileselector(ui_table, script_engine, script_engine_parent, path);
	bind_dataengineinput(lua, ui_table, script_engine, script_engine_parent, path);
	bind_hline(ui_table, script_engine, script_engine_parent);
	bind_image(ui_table, script_engine, script_engine_parent, path);
	bind_isotopesourceselector(ui_table, script_engine, script_engine_parent);
	bind_label(ui_table, script_engine, script_engine_parent);
	bind_lineedit(ui_table, script_engine, script_engine_parent);
	bind_plot(ui_table, script_engine, script_engine_parent);
	bind_polldataengine(ui_table, script_engine, script_engine_parent);
	bind_progressbar(ui_table, script_engine, script_engine_parent);
	bind_spinbox(ui_table, script_engine, script_engine_parent);
	bind_userinstructionlabel(ui_table, script_engine, script_engine_parent);
	bind_userwaitlabel(ui_table, script_engine, script_engine_parent);
	//bind other objects
	bind_chargecounter(lua);
	bind_datalogger(lua, script_engine_console_plaintext, path);
	bind_lua_functions(lua, ui_table, path, script_engine, script_engine_console_plaintext);
	//protocols and devices
	bind_scpiprotocol(lua, script_engine);
	bind_sg04countprotocol(lua);
	bind_manualprotocol(lua);
}
