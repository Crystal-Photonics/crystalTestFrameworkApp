QT = gui core network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SRC_DIR = $$PWD

INCLUDEPATH += $$PWD/src

INCLUDEPATH += $$PWD/libs/luasol/include
LIBS += -L$$PWD/libs/luasol
LIBS += -llua53

QPROTOCOL_INTERPRETER_PATH=$$PWD/libs/qRPCRuntimeParser
include($$QPROTOCOL_INTERPRETER_PATH/qProtocollInterpreter_static.pri)

QMAKE_CXXFLAGS += -std=c++14
CONFIG += c++14

CONFIG += warn
QMAKE_CXXFLAGS += -Werror
