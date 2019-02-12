include(../defaults.pri)
DESTDIR = $$BINDIR

CONFIG(debug, debug|release) {
    TARGET = crystalTestFrameworkAppd
} else {
    TARGET = crystalTestFrameworkApp
}

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

win32 {
	QMAKE_PRE_LINK += if not exist $$shell_path($$PWD/../libs/googletest/build) mkdir $$shell_path($$PWD/../libs/googletest/build) && cd $$shell_path($$PWD/../libs/googletest/build) && cmake .. && cmake --build .
}else{
	QMAKE_PRE_LINK += mkdir -p $$PWD/../libs/googletest/build && cd $$PWD/../libs/googletest/build && cmake .. && cmake --build .
}

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
	LuaUI/checkbox.h \
	LuaUI/color.h \
	LuaUI/combobox.h \
	LuaUI/combofileselector.h \
	LuaUI/dataengineinput.h \
	LuaUI/hline.h \
	LuaUI/image.h \
	LuaUI/isotopesourceselector.h \
	LuaUI/label.h \
	LuaUI/lineedit.h \
	LuaUI/plot.h \
	LuaUI/polldataengine.h \
	LuaUI/progressbar.h \
	LuaUI/spinbox.h \
	LuaUI/userinstructionlabel.h \
	LuaUI/userwaitlabel.h \
	LuaUI/window.h \
	Protocols/manualprotocol.h \
	Protocols/protocol.h \
	Protocols/rpcprotocol.h \
	Protocols/scpiprotocol.h \
	Protocols/sg04countprotocol.h \
	Windows/devicematcher.h \
	Windows/dummydatacreator.h \
	Windows/exceptiontalapprovaldialog.h \
	Windows/infowindow.h \
	Windows/mainwindow.h \
	Windows/scpimetadatadeviceselector.h \
	Windows/settingsform.h \
	chargecounter.h \
	communication_devices.h \
	communication_logger/communication_logger.h \
	config.h \
	console.h \
	data_engine/data_engine.h \
	data_engine/exceptionalapproval.h \
	datalogger.h \
	device_protocols_settings.h \
	deviceworker.h \
	environmentvariables.h \
	export.h \
	favorite_scripts.h \
	identicon/identicon.h \
	lua_functions.h \
	qt_util.h \
	scpimetadata.h \
	scriptengine.h \
	scriptsetup.h \
	testdescriptionloader.h \
	testrunner.h \
	ui_container.h \
	userentrystorage.h \
	util.h \
    forward_decls.h

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
	LuaUI/checkbox.cpp \
	LuaUI/color.cpp \
	LuaUI/combobox.cpp \
	LuaUI/combofileselector.cpp \
	LuaUI/dataengineinput.cpp \
	LuaUI/hline.cpp \
	LuaUI/image.cpp \
	LuaUI/isotopesourceselector.cpp \
	LuaUI/label.cpp \
	LuaUI/lineedit.cpp \
	LuaUI/plot.cpp \
	LuaUI/polldataengine.cpp \
	LuaUI/progressbar.cpp \
	LuaUI/spinbox.cpp \
	LuaUI/userinstructionlabel.cpp \
	LuaUI/userwaitlabel.cpp \
	LuaUI/window.cpp \
	Protocols/manualprotocol.cpp \
	Protocols/protocol.cpp \
	Protocols/rpcprotocol.cpp \
	Protocols/scpiprotocol.cpp \
	Protocols/sg04countprotocol.cpp \
	Windows/devicematcher.cpp \
	Windows/dummydatacreator.cpp \
	Windows/exceptiontalapprovaldialog.cpp \
	Windows/infowindow.cpp \
	Windows/mainwindow.cpp \
	Windows/scpimetadatadeviceselector.cpp \
	Windows/settingsform.cpp \
	chargecounter.cpp \
	communication_devices.cpp \
	communication_logger/communication_logger.cpp \
	config.cpp \
	console.cpp \
	data_engine/data_engine.cpp \
	data_engine/exceptionalapproval.cpp \
	datalogger.cpp \
	device_protocols_settings.cpp \
	deviceworker.cpp \
	environmentvariables.cpp \
	favorite_scripts.cpp \
	identicon/identicon.cpp \
	lua_functions.cpp \
	qt_util.cpp \
	scpimetadata.cpp \
	scriptengine.cpp \
	scriptsetup.cpp \
	testdescriptionloader.cpp \
	testrunner.cpp \
	ui_container.cpp \
	userentrystorage.cpp \
	util.cpp \
    forward_decls.cpp

win32 {
    system($$system_quote($$SH) $$PWD/../git_win.sh)
}else{
    system($$system_quote($$SH) $$PWD/../git_linux.sh)
}

RESOURCES += \
    ../resources.qrc

DISTFILES += \
    ../examples/scripts/example/Jellyfish.jpg \
    ../examples/scripts/example/dummy.txt \
    ../examples/scripts/example/chargeCounter_test.lua \
    ../examples/scripts/example/file_selector_test.lua \
    ../examples/scripts/example/luatests.lua \
    ../examples/scripts/example/manual_device_test.lua \
    ../examples/scripts/example/nuklid_selector_test.lua \
    ../examples/scripts/example/scpi_hm8150_test.lua \
    ../examples/scripts/example/user_wait_label.lua

