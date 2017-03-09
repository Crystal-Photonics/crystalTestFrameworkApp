include(../defaults.pri)
CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

INCLUDEPATH += $$OUT_PWD/..

INCLUDEPATH += $$OUT_PWD

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

UI_DIR = $$PWD

FORMS += \
	mainwindow.ui \
    pathsettingswindow.ui
