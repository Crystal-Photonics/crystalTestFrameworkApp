include(../defaults.pri)
DESTDIR = $$BINDIR

TEMPLATE = app
SOURCES += main.cpp
DEFINES += EXPORT_APPLICATION
TARGET = crystalTestFramework
LIBS += -L$$BINDIR

#RC_FILE = resources.rc
#RC_ICONS += ../src/icons/app_icon_16.ico
#RC_ICONS += ../src/icons/app_icon_24.ico
#RC_ICONS += ../src/icons/app_icon_32.ico
#RC_ICONS += ../src/icons/app_icon_64.ico
RC_ICONS += ../src/icons/app_icon_multisize.ico
#RC_ICONS += ../src/icons/app_icon_256.ico

CONFIG(debug, debug|release) {
    LIBS += -lcrystalTestFrameworkAppd
} else {
    LIBS += -lcrystalTestFrameworkApp
}


