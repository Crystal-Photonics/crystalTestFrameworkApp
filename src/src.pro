include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

SOURCES += mainwindow.cpp \
    scriptengine.cpp

HEADERS += mainwindow.h \
    scriptengine.h
HEADERS += commodulinterface.h


FORMS    += mainwindow.ui

INCLUDEPATH += $$(PYTHON_PATH)/include

!exists( $$(PYTHON_PATH)/include/python.h ) {
    error (Python directory needs to be configured in environment variable PYTHON_PATH. eg. C:/Python27 )
}
