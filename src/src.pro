include(../defaults.pri)
DESTDIR = $$BINDIR

include (../libs/QtRPT/QtRPT.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

FORMS += \
	mainwindow.ui \
	pathsettingswindow.ui \
    devicematcher.ui \
    scpimetadatadeviceselector.ui

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
	mainwindow.h \
	pathsettingswindow.h \
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
    devicematcher.h \
    scpimetadatadeviceselector.h \
    Protocols/sg04countprotocol.h

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
	mainwindow.cpp \
	pathsettingswindow.cpp \
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
    devicematcher.cpp \
    scpimetadatadeviceselector.cpp \
    Protocols/sg04countprotocol.cpp
