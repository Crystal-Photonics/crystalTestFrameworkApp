include(../defaults.pri)
DESTDIR = $$BINDIR

TEMPLATE = app
SOURCES += main.cpp
DEFINES += EXPORT_APPLICATION
TARGET = crystalTestFramework
LIBS += -L$$BINDIR

CONFIG(debug, debug|release) {
    LIBS += -lcrystalTestFrameworkAppd
} else {
    LIBS += -lcrystalTestFrameworkApp
}


