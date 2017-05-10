include(../defaults.pri)
DESTDIR = $$BINDIR

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY




FORMS += \
	Windows/mainwindow.ui \
	Windows/pathsettingswindow.ui \
	Windows/devicematcher.ui \
	Windows/scpimetadatadeviceselector.ui \
	Windows/hotkey_picker.ui

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
	config.h \
	console.h \
	data_engine/data_engine.h \
	deviceworker.h \
	export.h \
	lua_functions.h \
	LuaUI/button.h \
	LuaUI/color.h \
	LuaUI/lineedit.h \
	LuaUI/plot.h \
	LuaUI/window.h \
	Windows/mainwindow.h \
	Windows/pathsettingswindow.h \
	Protocols/protocol.h \
	Protocols/rpcprotocol.h \
	Protocols/scpiprotocol.h \
	qt_util.h \
	scriptengine.h \
	testdescriptionloader.h \
	testrunner.h \
	util.h \
    device_protocols_settings.h \
    scpimetadata.h \
	Windows/devicematcher.h \
	Windows/scpimetadatadeviceselector.h \
	Protocols/sg04countprotocol.h \
        Windows/hotkey_picker.h \
    datalogger.h \
    LuaUI/isotopesourceselector.h \
    LuaUI/combofileselector.h \
    chargecounter.h \
    LuaUI/combobox.h \
    LuaUI/checkbox.h \
    LuaUI/label.h \
    LuaUI/image.h \
    ui_container.h \
    Protocols/manualprotocol.h \
    CommunicationDevices/dummycommunicationdevice.h \
    CommunicationDevices/usbtmccommunicationdevice.h \
    CommunicationDevices/usbtmc_libusb.h \
    CommunicationDevices/libusb_base.h \
    CommunicationDevices/usbtmc.h \
    CommunicationDevices/libusbscan.h

SOURCES += \
	CommunicationDevices/communicationdevice.cpp \
	CommunicationDevices/comportcommunicationdevice.cpp \
	CommunicationDevices/echocommunicationdevice.cpp \
	CommunicationDevices/socketcommunicationdevice.cpp \
	config.cpp \
	console.cpp \
	data_engine/data_engine.cpp \
	deviceworker.cpp \
	lua_functions.cpp \
	LuaUI/button.cpp \
	LuaUI/color.cpp \
	LuaUI/lineedit.cpp \
	LuaUI/plot.cpp \
	LuaUI/window.cpp \
	Windows/mainwindow.cpp \
	Windows/pathsettingswindow.cpp \
	Protocols/protocol.cpp \
	Protocols/rpcprotocol.cpp \
	Protocols/scpiprotocol.cpp \
	qt_util.cpp \
	scriptengine.cpp \
	testdescriptionloader.cpp \
	testrunner.cpp \
	util.cpp \
    device_protocols_settings.cpp \
    scpimetadata.cpp \
	Windows/devicematcher.cpp \
	Windows/scpimetadatadeviceselector.cpp \
	Protocols/sg04countprotocol.cpp \
        Windows/hotkey_picker.cpp \
    datalogger.cpp \
    LuaUI/isotopesourceselector.cpp \
    LuaUI/combofileselector.cpp \
    chargecounter.cpp \
    LuaUI/combobox.cpp \
    LuaUI/checkbox.cpp \
    LuaUI/label.cpp \
    LuaUI/image.cpp \
    ui_container.cpp \
    Protocols/manualprotocol.cpp \
    CommunicationDevices/dummycommunicationdevice.cpp \
    CommunicationDevices/usbtmccommunicationdevice.cpp \
    CommunicationDevices/usbtmc_libusb.cpp  \
    CommunicationDevices/libusb_base.cpp \
    CommunicationDevices/usbtmc.cpp \
    CommunicationDevices/libusbscan.cpp
