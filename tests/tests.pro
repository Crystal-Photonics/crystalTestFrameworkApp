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
	testScriptEngine.h \
    testreporthistory.h

SOURCES += \
	test_data_engine.cpp \
	main.cpp \
        testgooglemock.cpp \
	testqstring.cpp \
	testScriptEngine.cpp \
    testreporthistory.cpp

win32 {
        QMAKE_PRE_LINK += if not exist $$shell_path($$PWD/../libs/googletest/build) mkdir $$shell_path($$PWD/../libs/googletest/build) && cd $$shell_path($$PWD/../libs/googletest/build) && cmake .. && cmake --build .
}else{
        QMAKE_PRE_LINK += mkdir -p $$PWD/../libs/googletest/build && cd $$PWD/../libs/googletest/build && cmake .. && cmake --build .
}

INCLUDEPATH += $$PWD/../libs/googletest/googletest/include
INCLUDEPATH += $$PWD/../libs/googletest/googlemock/include

LIBS += -L$$BINDIR

LIBS += -L$$PWD/../libs/googletest/build/lib
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


