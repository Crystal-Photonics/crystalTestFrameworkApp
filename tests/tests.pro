
include(../defaults.pri)

TEMPLATE = app
CONFIG += console
QT +=  testlib

SOURCES += main.cpp


#warning(qt = $$QT)

#CONFIG( debug, debug|release ) {
#    # debug
#    QMAKE_LIBDIR += "../src/debug"
#} else {
#    # release
#    QMAKE_LIBDIR += "../src/release"
#}


LIBS += -L../src/debug -lcrystalTestFrameworkApp

