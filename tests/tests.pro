include(../defaults.pri)
DESTDIR = $$BINDIR

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

LIBS += -L$$BINDIR
LIBS += -L$$PWD/../libs/googletest/build
LIBS += -lgmock
LIBS += -lgtest


win32 {
    system($$system_quote($$SH) $$PWD/../git_win.sh)
}else{
    system($$system_quote($$SH) $$PWD/../git_linux.sh)
}


CONFIG(debug, debug|release) {
    LIBS += -lcrystalTestFrameworkAppd
} else {
    LIBS += -lcrystalTestFrameworkApp
}

COPY_DIR = "$$(UNIXTOOLS)cp -r"

#copies scripts into builds

#runtests.commands = $$RUNTEST
#runtests.depends = copydata

copydata.commands = $$COPY_DIR $$PWD/scripts $$OUT_PWD/
first.depends = $(first) copydata

export(first.depends)
export(copydata.commands)

QMAKE_EXTRA_TARGETS += first copydata
