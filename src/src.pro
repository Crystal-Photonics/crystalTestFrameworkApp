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
	Windows/mainwindow.ui \
	Windows/devicematcher.ui \
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
	CommunicationDevices/echocommunicationdevice.h \
	CommunicationDevices/socketcommunicationdevice.h \
	LuaUI/button.h \
	LuaUI/color.h \
	LuaUI/lineedit.h \
	LuaUI/plot.h \
	LuaUI/window.h \
	Protocols/protocol.h \
	Protocols/rpcprotocol.h \
	Protocols/scpiprotocol.h \
	Windows/devicematcher.h \
	Windows/mainwindow.h \
	Windows/scpimetadatadeviceselector.h \
	config.h \
	console.h \
	data_engine/data_engine.h \
	deviceworker.h \
	export.h \
	lua_functions.h \
	qt_util.h \
	scriptengine.h \
	testdescriptionloader.h \
	testrunner.h \
	util.h \
	CommunicationDevices/dummycommunicationdevice.h \
	CommunicationDevices/libusb_base.h \
	CommunicationDevices/libusbscan.h \
	CommunicationDevices/rpcserialport.h \
	CommunicationDevices/usbtmc.h \
	CommunicationDevices/usbtmc_libusb.h \
	CommunicationDevices/usbtmccommunicationdevice.h \
	LuaUI/checkbox.h \
	LuaUI/combobox.h \
	LuaUI/combofileselector.h \
	LuaUI/dataengineinput.h \
	LuaUI/hline.h \
	LuaUI/image.h \
	LuaUI/isotopesourceselector.h \
	LuaUI/label.h \
	LuaUI/polldataengine.h \
	LuaUI/progressbar.h \
	LuaUI/spinbox.h \
	LuaUI/userinstructionlabel.h \
	LuaUI/userwaitlabel.h \
	Protocols/manualprotocol.h \
	Protocols/sg04countprotocol.h \
	Windows/dummydatacreator.h \
	Windows/settingsform.h \
	chargecounter.h \
	communication_devices.h \
	data_engine/exceptionalapproval.h \
	datalogger.h \
	device_protocols_settings.h \
	environmentvariables.h \
	favorite_scripts.h \
	identicon/identicon.h \
	scpimetadata.h \
	scriptsetup.h \
	ui_container.h \
    userentrystorage.h \
    Windows/reporthistoryquery.h


HEADERS +=    Windows/exceptiontalapprovaldialog.h
#HEADERS +=    data_engine/data_engine_strings.h
HEADERS +=    Windows/infowindow.h

SOURCES += \
	CommunicationDevices/communicationdevice.cpp \
	CommunicationDevices/comportcommunicationdevice.cpp \
	CommunicationDevices/echocommunicationdevice.cpp \
	CommunicationDevices/socketcommunicationdevice.cpp \
	LuaUI/button.cpp \
	LuaUI/color.cpp \
	LuaUI/lineedit.cpp \
	LuaUI/plot.cpp \
	LuaUI/window.cpp \
	Protocols/protocol.cpp \
	Protocols/rpcprotocol.cpp \
	Protocols/scpiprotocol.cpp \
	Protocols/sg04countprotocol.cpp \
	Windows/devicematcher.cpp \
	Windows/mainwindow.cpp \
	Windows/scpimetadatadeviceselector.cpp \
	config.cpp \
	console.cpp \
	data_engine/data_engine.cpp \
	deviceworker.cpp \
	lua_functions.cpp \
	qt_util.cpp \
	scriptengine.cpp \
	testdescriptionloader.cpp \
	testrunner.cpp \
	util.cpp \
	CommunicationDevices/dummycommunicationdevice.cpp \
	CommunicationDevices/libusb_base.cpp \
	CommunicationDevices/libusbscan.cpp \
	CommunicationDevices/rpcserialport.cpp \
	CommunicationDevices/usbtmc.cpp \
	CommunicationDevices/usbtmc_libusb.cpp  \
	CommunicationDevices/usbtmccommunicationdevice.cpp \
	LuaUI/checkbox.cpp \
	LuaUI/combobox.cpp \
	LuaUI/combofileselector.cpp \
	LuaUI/dataengineinput.cpp \
	LuaUI/hline.cpp \
	LuaUI/image.cpp \
	LuaUI/isotopesourceselector.cpp \
	LuaUI/label.cpp \
	LuaUI/polldataengine.cpp \
	LuaUI/progressbar.cpp \
	LuaUI/spinbox.cpp \
	LuaUI/userinstructionlabel.cpp \
	LuaUI/userwaitlabel.cpp \
	Protocols/manualprotocol.cpp \
	Windows/dummydatacreator.cpp \
	Windows/exceptiontalapprovaldialog.cpp \
	Windows/infowindow.cpp \
	Windows/settingsform.cpp \
	chargecounter.cpp \
	communication_devices.cpp \
	data_engine/exceptionalapproval.cpp \
	datalogger.cpp \
	device_protocols_settings.cpp \
	environmentvariables.cpp \
	favorite_scripts.cpp \
	identicon/identicon.cpp \
	scpimetadata.cpp \
	scriptsetup.cpp \
	ui_container.cpp \
    userentrystorage.cpp \
    Windows/reporthistoryquery.cpp

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

