include(../defaults.pri)

TEMPLATE = app
CONFIG += console
QT += testlib

DEFINES += EXPORT_APPLICATION

HEADERS += \
	test_data_engine.h \
	autotest.h \
	testgooglemock.h \
	testqstring.h \
	testScriptEngine.h

SOURCES += \
	test_data_engine.cpp \
	main.cpp \
	testgooglemock.cpp \
	testqstring.cpp \
	testScriptEngine.cpp


INCLUDEPATH += $$PWD/../libs/googletest/googletest/include
INCLUDEPATH += $$PWD/../libs/googletest/googlemock/include

LIBS += -L$$PWD/../libs/googletest/build
LIBS += -lgmock
LIBS += -lgtest
LIBS += -lcrystalTestFrameworkApp


LIBS += -L$$OUT_PWD/../src

CONFIG( debug, debug|release ) {
    # debug
    LIBS += -L$$OUT_PWD/../src/debug
} else {
    # release
    LIBS += -L$$OUT_PWD/../src/release
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

QMAKE_EXTRA_TARGETS += first copydata
