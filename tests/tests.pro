
include(../defaults.pri)

TEMPLATE = app
CONFIG += console
#CONFIG += qtestlib
QT +=  testlib

SOURCES += main.cpp
SOURCES += testScriptEngine.cpp
#sources.files = $$SOURCES *.pro

#warning(qt = $$QT)

CONFIG( debug, debug|release ) {
    # debug
    #QMAKE_LIBDIR += "../src/debug"
    LIBS += -L../src/debug
} else {
    # release
    #QMAKE_LIBDIR += "../src/release"
    LIBS += -L../src/release
}

COPY_DIR = "$$(UNIXTOOLS)cp -r"

#!exists($$(UNIXTOOLS)cp.exe) {
    #message ( $$(UNIXTOOLS)cp)
    #error (UNIXTOOLS directory needs to be configured in environment variable UNIXTOOLS. eg. C:\Program Files (x86)\Git\bin )
#}

copydata.commands = $$COPY_DIR $$PWD/scripts $$OUT_PWD/
#message ($$COPY_DIR $$PWD/tests/scripts $$OUT_PWD/scripts)
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS +=   first copydata

LIBS +=  -lcrystalTestFrameworkApp
#
