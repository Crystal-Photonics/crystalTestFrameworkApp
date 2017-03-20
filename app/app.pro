include(../defaults.pri)
DESTDIR = $$BINDIR

TEMPLATE = app
SOURCES += main.cpp
DEFINES += EXPORT_APPLICATION

LIBS += -L$$BINDIR
LIBS +=  -lcrystalTestFrameworkApp
