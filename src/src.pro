include(../defaults.pri)
DESTDIR = $$BINDIR

CONFIG(debug, debug|release) {
    TARGET = crystalTestFrameworkAppd
} else {
    TARGET = crystalTestFrameworkApp
}

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY



FORMS += \
	Windows/devicematcher.ui \
	Windows/mainwindow.ui \
	Windows/scpimetadatadeviceselector.ui \
	Windows/dummydatacreator.ui \
	Windows/exceptiontalapprovaldialog.ui \
	Windows/infowindow.ui \
    Windows/settingsform.ui \
    Windows/reporthistoryquery.ui

QPROTOCOL_INTERPRETER_PATH=$$PWD/../libs/qRPCRuntimeParser
INCLUDEPATH += $$QPROTOCOL_INTERPRETER_PATH/project/src
include($$QPROTOCOL_INTERPRETER_PATH/qProtocollInterpreter_static.pri)

LIBS += -L$$BINDIR

#QMAKE_CXXFLAGS += --time-report
#QMAKE_CXXFLAGS += -flto
#QMAKE_LFLAGS += -fno-use-linker-plugin -flto

HEADERS += \
	CommunicationDevices/communicationdevice.h \
	CommunicationDevices/comportcommunicationdevice.h \
	CommunicationDevices/dummycommunicationdevice.h \
	CommunicationDevices/echocommunicationdevice.h \
	CommunicationDevices/libusb_base.h \
	CommunicationDevices/libusbscan.h \
	CommunicationDevices/rpcserialport.h \
	CommunicationDevices/socketcommunicationdevice.h \
	CommunicationDevices/usbtmc.h \
	CommunicationDevices/usbtmc_libusb.h \
	CommunicationDevices/usbtmccommunicationdevice.h \
	LuaUI/button.h \
	LuaUI/button_lua.h \
	LuaUI/checkbox.h \
	LuaUI/checkbox_lua.h \
	LuaFunctions/color.h \
	LuaFunctions/color_lua.h \
	LuaUI/combobox.h \
	LuaUI/combobox_lua.h \
	LuaUI/combofileselector.h \
	LuaUI/combofileselector_lua.h \
	LuaFunctions/dataengineinput.h \
	LuaFunctions/dataengineinput_lua.h \
	LuaUI/hline.h \
	LuaUI/hline_lua.h \
	LuaUI/image.h \
	LuaUI/image_lua.h \
	LuaUI/isotopesourceselector.h \
	LuaUI/isotopesourceselector_lua.h \
	LuaUI/label.h \
	LuaUI/label_lua.h \
	LuaUI/lineedit.h \
	LuaUI/lineedit_lua.h \
	LuaUI/plot.h \
	LuaUI/plot_lua.h \
        LuaFunctions/polldataengine.h \
        LuaFunctions/polldataengine_lua.h \
        LuaFunctions/moving_average_lua.h \
        LuaFunctions/moving_average.h \
	LuaUI/progressbar.h \
	LuaUI/progressbar_lua.h \
	LuaUI/spinbox.h \
	LuaUI/spinbox_lua.h \
	LuaUI/userinstructionlabel.h \
	LuaUI/userinstructionlabel_lua.h \
	LuaUI/userwaitlabel.h \
	LuaUI/userwaitlabel_lua.h \
	LuaUI/window.h \
	Protocols/manualprotocol.h \
	Protocols/manualprotocol_lua.h \
	Protocols/protocol.h \
	Protocols/rpcprotocol.h \
	Protocols/scpiprotocol.h \
	Protocols/scpiprotocol_lua.h \
	Protocols/sg04countprotocol.h \
	Protocols/sg04countprotocol_lua.h \
	Windows/devicematcher.h \
	Windows/dummydatacreator.h \
	Windows/exceptiontalapprovaldialog.h \
	Windows/infowindow.h \
	Windows/mainwindow.h \
	Windows/plaintextedit.h \
	Windows/reporthistoryquery.h \
	Windows/scpimetadatadeviceselector.h \
	Windows/settingsform.h \
	LuaFunctions/chargecounter.h \
	LuaFunctions/chargecounter_lua.h \
	communication_devices.h \
	communication_logger/communication_logger.h \
	config.h \
	console.h \
	data_engine/data_engine.h \
	data_engine/exceptionalapproval.h \
	LuaFunctions/datalogger.h \
	LuaFunctions/datalogger_lua.h \
	device_protocols_settings.h \
	deviceworker.h \
	environmentvariables.h \
	exception_wrap.h \
	export.h \
	favorite_scripts.h \
	forward_decls.h \
	identicon/identicon.h \
	LuaFunctions/lua_functions.h \
	LuaFunctions/lua_functions_lua.h \
	qt_util.h \
	scpimetadata.h \
	scriptengine.h \
	scriptsetup.h \
	scriptsetup_helper.h \
	testdescriptionloader.h \
	testrunner.h \
	thread_pool.h \
	ui_container.h \
	userentrystorage.h \
	util.h


SOURCES += \
	CommunicationDevices/communicationdevice.cpp \
	CommunicationDevices/comportcommunicationdevice.cpp \
	CommunicationDevices/dummycommunicationdevice.cpp \
	CommunicationDevices/echocommunicationdevice.cpp \
	CommunicationDevices/libusb_base.cpp \
	CommunicationDevices/libusbscan.cpp \
	CommunicationDevices/rpcserialport.cpp \
	CommunicationDevices/socketcommunicationdevice.cpp \
	CommunicationDevices/usbtmc.cpp \
	CommunicationDevices/usbtmc_libusb.cpp  \
	CommunicationDevices/usbtmccommunicationdevice.cpp \
	LuaUI/button.cpp \
	LuaUI/button_lua.cpp \
	LuaUI/checkbox.cpp \
	LuaUI/checkbox_lua.cpp \
	LuaFunctions/color.cpp \
	LuaFunctions/color_lua.cpp \
	LuaUI/combobox.cpp \
	LuaUI/combobox_lua.cpp \
	LuaUI/combofileselector.cpp \
	LuaUI/combofileselector_lua.cpp \
	LuaFunctions/dataengineinput.cpp \
	LuaFunctions/dataengineinput_lua.cpp \
	LuaUI/hline.cpp \
	LuaUI/hline_lua.cpp \
	LuaUI/image.cpp \
	LuaUI/image_lua.cpp \
	LuaUI/isotopesourceselector.cpp \
	LuaUI/isotopesourceselector_lua.cpp \
	LuaUI/label.cpp \
	LuaUI/label_lua.cpp \
	LuaUI/lineedit.cpp \
	LuaUI/lineedit_lua.cpp \
	LuaUI/plot.cpp \
	LuaUI/plot_lua.cpp \
	LuaFunctions/polldataengine.cpp \
	LuaFunctions/polldataengine_lua.cpp \
	LuaUI/progressbar.cpp \
	LuaUI/progressbar_lua.cpp \
	LuaUI/spinbox.cpp \
	LuaUI/spinbox_lua.cpp \
	LuaUI/userinstructionlabel.cpp \
	LuaUI/userinstructionlabel_lua.cpp \
	LuaUI/userwaitlabel.cpp \
	LuaUI/userwaitlabel_lua.cpp \
	LuaUI/window.cpp \
	Protocols/manualprotocol.cpp \
	Protocols/manualprotocol_lua.cpp \
	Protocols/protocol.cpp \
	Protocols/rpcprotocol.cpp \
	Protocols/scpiprotocol.cpp \
	Protocols/scpiprotocol_lua.cpp \
	Protocols/sg04countprotocol.cpp \
	Protocols/sg04countprotocol_lua.cpp \
	Windows/devicematcher.cpp \
	Windows/dummydatacreator.cpp \
	Windows/exceptiontalapprovaldialog.cpp \
	Windows/infowindow.cpp \
	Windows/mainwindow.cpp \
	Windows/plaintextedit.cpp \
	Windows/reporthistoryquery.cpp \
	Windows/scpimetadatadeviceselector.cpp \
	Windows/settingsform.cpp \
	LuaFunctions/chargecounter.cpp \
	LuaFunctions/chargecounter_lua.cpp \
	communication_devices.cpp \
	communication_logger/communication_logger.cpp \
	config.cpp \
	console.cpp \
	data_engine/data_engine.cpp \
	data_engine/exceptionalapproval.cpp \
	LuaFunctions/datalogger.cpp \
	LuaFunctions/datalogger_lua.cpp \
	device_protocols_settings.cpp \
	deviceworker.cpp \
	environmentvariables.cpp \
	exception_wrap.cpp \
	favorite_scripts.cpp \
	forward_decls.cpp \
	identicon/identicon.cpp \
	LuaFunctions/lua_functions.cpp \
        LuaFunctions/lua_functions_lua.cpp \
        LuaFunctions/moving_average_lua.cpp \
        LuaFunctions/moving_average.cpp \
	qt_util.cpp \
	scpimetadata.cpp \
	scriptengine.cpp \
	scriptsetup.cpp \
	scriptsetup_helper.cpp \
	testdescriptionloader.cpp \
	testrunner.cpp \
	thread_pool.cpp \
	ui_container.cpp \
	userentrystorage.cpp \
	util.cpp

win32 {
    system($$system_quote($$SH) $$PWD/../git_win.sh)
}else{
    system($$system_quote($$SH) $$PWD/../git_linux.sh)
}

RESOURCES += \
    ../resources.qrc

DISTFILES += \
	../examples/scripts/example/Jellyfish.jpg \
	../examples/scripts/example/chargeCounter_test.lua \
	../examples/scripts/example/dummy.txt \
	../examples/scripts/example/file_selector_test.lua \
	../examples/scripts/example/luatests.lua \
	../examples/scripts/example/manual_device_test.lua \
	../examples/scripts/example/nuklid_selector_test.lua \
	../examples/scripts/example/scpi_hm8150_test.lua \
	../examples/scripts/example/user_wait_label.lua
