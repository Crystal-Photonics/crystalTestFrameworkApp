CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QT = gui core network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SRC_DIR = $$PWD

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/libs/luasol/include
LIBS += -L$$PWD/libs/luasol
LIBS += -llua53

QWT_DIR = $$PWD/libs/qwt
INCLUDEPATH += $$QWT_DIR/include
LIBS += -L$$QWT_DIR/lib
Debug:LIBS += -lqwtd
Release:LIBS += -lqwt

DEFINES += SOL_CHECK_ARGUMENTS

QPROTOCOL_INTERPRETER_PATH=$$PWD/libs/qRPCRuntimeParser
INCLUDEPATH += $$QPROTOCOL_INTERPRETER_PATH/project/src
include($$QPROTOCOL_INTERPRETER_PATH/qProtocollInterpreter_static.pri)

QMAKE_CXXFLAGS += -Werror

QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
QMAKE_CXXFLAGS_RELEASE += -Wunused-function -Wunused-parameter -Wunused-variable

QMAKE_CXXFLAGS_DEBUG += -g -fno-omit-frame-pointer
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...

HEADERS += \
	$$PWD/src/CommunicationDevices/communicationdevice.h \
	$$PWD/src/CommunicationDevices/comportcommunicationdevice.h \
	$$PWD/src/CommunicationDevices/echocommunicationdevice.h \
	$$PWD/src/CommunicationDevices/socketcommunicationdevice.h \
	$$PWD/src/config.h \
	$$PWD/src/console.h \
	$$PWD/src/data_engine/data_engine.h \
	$$PWD/src/deviceworker.h \
	$$PWD/src/export.h \
	$$PWD/src/lua_functions.h \
	$$PWD/src/LuaUI/button.h \
	$$PWD/src/LuaUI/color.h \
	$$PWD/src/LuaUI/lineedit.h \
	$$PWD/src/LuaUI/plot.h \
	$$PWD/src/LuaUI/window.h \
	$$PWD/src/mainwindow.h \
	$$PWD/src/pathsettingswindow.h \
	$$PWD/src/Protocols/protocol.h \
	$$PWD/src/Protocols/rpcprotocol.h \
	$$PWD/src/qt_util.h \
	$$PWD/src/scriptengine.h \
	$$PWD/src/testdescriptionloader.h \
	$$PWD/src/testrunner.h \
	$$PWD/src/util.h

SOURCES += \
	$$PWD/src/CommunicationDevices/communicationdevice.cpp \
	$$PWD/src/CommunicationDevices/comportcommunicationdevice.cpp \
	$$PWD/src/CommunicationDevices/echocommunicationdevice.cpp \
	$$PWD/src/CommunicationDevices/socketcommunicationdevice.cpp \
	$$PWD/src/config.cpp \
	$$PWD/src/console.cpp \
	$$PWD/src/data_engine/data_engine.cpp \
	$$PWD/src/deviceworker.cpp \
	$$PWD/src/lua_functions.cpp \
	$$PWD/src/LuaUI/button.cpp \
	$$PWD/src/LuaUI/color.cpp \
	$$PWD/src/LuaUI/lineedit.cpp \
	$$PWD/src/LuaUI/plot.cpp \
	$$PWD/src/LuaUI/window.cpp \
	$$PWD/src/mainwindow.cpp \
	$$PWD/src/pathsettingswindow.cpp \
	$$PWD/src/Protocols/protocol.cpp \
	$$PWD/src/Protocols/rpcprotocol.cpp \
	$$PWD/src/qt_util.cpp \
	$$PWD/src/scriptengine.cpp \
	$$PWD/src/testdescriptionloader.cpp \
	$$PWD/src/testrunner.cpp \
	$$PWD/src/util.cpp
