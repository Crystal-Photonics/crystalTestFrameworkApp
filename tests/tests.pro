
include(../defaults.pri)

TEMPLATE = app
CONFIG += console
QT +=  testlib

HEADERS += autotest.h \
    testqstring.h
SOURCES += main.cpp \
    testqstring.cpp

SOURCES += testScriptEngine.cpp
HEADERS += testScriptEngine.h


LIBS += -lgmock
LIBS += -lgtest


CONFIG( debug, debug|release ) {
    # debug
    LIBS += -L../src/debug
} else {
    # release
    LIBS += -L../src/release
}

COPY_DIR = "$$(UNIXTOOLS)cp -r"

message($$COPY_DIR)
message($$OUT_PWD/)
message($$PWD/scripts)

#copies scripts into builds

#runtests.commands = $$RUNTEST
#runtests.depends = copydata

copydata.commands = $$COPY_DIR $$PWD/scripts $$OUT_PWD/
first.depends = $(first) copydata

export(first.depends)
export(copydata.commands)

QMAKE_EXTRA_TARGETS +=   first copydata

LIBS +=  -lcrystalTestFrameworkApp


#
