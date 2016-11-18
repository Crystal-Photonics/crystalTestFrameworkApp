QT = gui core network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SRC_DIR = $$PWD

INCLUDEPATH += $$PWD/src

INCLUDEPATH += $$PWD/libs/luasol/include
LIBS += -L$$PWD/libs/luasol
LIBS += -llua53

QPROTOCOL_INTERPRETER_PATH=$$PWD/libs/qRPCRuntimeParser
INCLUDEPATH += $$QPROTOCOL_INTERPRETER_PATH/project/src
include($$QPROTOCOL_INTERPRETER_PATH/qProtocollInterpreter_static.pri)

QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -Werror

QMAKE_CXXFLAGS_DEBUG += -g -fno-omit-frame-pointer
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...

CONFIG += c++14
CONFIG += warn
