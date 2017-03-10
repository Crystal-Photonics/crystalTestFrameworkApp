CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QT = gui core network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += $$PWD/src
INCLUDEPATH += ../libs/luasol/include

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
