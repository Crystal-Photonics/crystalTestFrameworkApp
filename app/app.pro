include(../defaults.pri)

TEMPLATE = app
SOURCES += main.cpp
DEFINES += EXPORT_APPLICATION

#on my system there is no xxx/debug or xxx/release path scheme. since we can define more paths we have
#the following line:
LIBS += -L$$OUT_PWD/../src/

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
