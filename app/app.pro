include(../defaults.pri)

TEMPLATE = app
SOURCES += main.cpp


CONFIG( debug, debug|release ) {
    # debug
    #QMAKE_LIBDIR += "../src/debug"
    LIBS += -L../src/debug
} else {
    # release
    #QMAKE_LIBDIR += "../src/release"
    LIBS += -L../src/release
}


LIBS +=  -lcrystalTestFrameworkApp
