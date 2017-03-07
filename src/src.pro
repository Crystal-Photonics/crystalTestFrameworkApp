include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

SOURCES += mainwindow.cpp \
    pathsettingswindow.cpp \
    config.cpp \
    Protocols/rpcprotocol.cpp \
    console.cpp \
    Protocols/protocol.cpp \
    qt_util.cpp \
	LuaUI/plot.cpp \
    deviceworker.cpp \
    testrunner.cpp \
    testdescriptionloader.cpp \
    LuaUI/button.cpp \
    LuaUI/lineedit.cpp \
    LuaUI/window.cpp \
    lua_functions.cpp \
    LuaUI/color.cpp
SOURCES += scriptengine.cpp
SOURCES += util.cpp
SOURCES += CommunicationDevices/communicationdevice.cpp
SOURCES += CommunicationDevices/comportcommunicationdevice.cpp
SOURCES += CommunicationDevices/echocommunicationdevice.cpp
SOURCES += CommunicationDevices/socketcommunicationdevice.cpp

HEADERS += mainwindow.h \
    pathsettingswindow.h \
    config.h \
    Protocols/rpcprotocol.h \
    console.h \
    Protocols/protocol.h \
    qt_util.h \
	LuaUI/plot.h \
    deviceworker.h \
    testrunner.h \
    testdescriptionloader.h \
    LuaUI/button.h \
    LuaUI/lineedit.h \
    LuaUI/window.h \
    lua_functions.h \
    LuaUI/color.h
HEADERS += scriptengine.h
HEADERS += export.h
HEADERS += util.h
HEADERS +=CommunicationDevices/communicationdevice.h
HEADERS +=CommunicationDevices/comportcommunicationdevice.h
HEADERS +=CommunicationDevices/echocommunicationdevice.h
HEADERS +=CommunicationDevices/socketcommunicationdevice.h

FORMS    += mainwindow.ui \
    pathsettingswindow.ui
