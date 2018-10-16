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

win32 {
    equals(GCC_MACHINE,  x86_64-w64-mingw32){
        LIBS += -L$$PWD/../libs/googletest/build/win64
    }
    equals(GCC_MACHINE, i686-w64-mingw32){
        LIBS += -L$$PWD/../libs/googletest/build/win32
    }

}else{
    LIBS += -L$$PWD/../libs/googletest/build/
}

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
#TEST1="123"
#TEST2="123"
#message($$TEST1)
#message($$TEST2)

#!equals(TEST1, $$TEST2) {
#    message("not equal")
#}else{
#   message("equal")
#}

!equals(OUT_PWD, $$PWD) {
    #avoid cp: ‘xx/scripts’ and ‘xx/scripts’ are the same file
    copydata.commands = $$COPY_DIR $$PWD/scripts $$OUT_PWD/
    first.depends = $(first) copydata

    export(first.depends)
    export(copydata.commands)

    QMAKE_EXTRA_TARGETS += first copydata
}


