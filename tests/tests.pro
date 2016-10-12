include(../defaults.pri)

TEMPLATE = app
CONFIG += console
QT +=  testlib

DEFINES += EXPORT_APPLICATION

HEADERS += autotest.h
HEADERS += testqstring.h
#HEADERS += CommunicationDevices/testcommunicationdevice.h
#HEADERS += CommunicationDevices/testechocommunicationdevice.h
#HEADERS += CommunicationDevices/testsocketcommunicationdevice.h
#HEADERS += CommunicationDevices/testcomportcommunicationdevice.h

SOURCES += main.cpp
SOURCES += testqstring.cpp
#SOURCES += CommunicationDevices/testcommunicationdevice.cpp
#SOURCES += CommunicationDevices/testechocommunicationdevice.cpp
#SOURCES += CommunicationDevices/testsocketcommunicationdevice.cpp
#SOURCES += CommunicationDevices/testcomportcommunicationdevice.cpp

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

#message($$COPY_DIR)
#message($$OUT_PWD/)
#message($$PWD/scripts)

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
