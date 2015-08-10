include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

SOURCES += mainwindow.cpp \
    scriptengine.cpp \
    pysys.cpp \
    communicationdevice.cpp \
    socketcommunicationdevice.cpp \
    echocommunicationdevice.cpp \
    util.cpp

HEADERS += mainwindow.h \
    scriptengine.h \
    pysys.h \
    communicationdevice.h \
    socketcommunicationdevice.h \
    echocommunicationdevice.h \
    export.h \
    util.h
HEADERS += commodulinterface.h


FORMS    += mainwindow.ui

INCLUDEPATH += $$(PYTHON_PATH)/include

exists( $$(PYTHON_PATH)/include/python.h ) {
    #message(found python windows)
    INCLUDEPATH += $$(PYTHON_PATH)/include
    PYTHON_FOUND = 1
}

exists( $$(PYTHON_PATH)/Python.h ) {
    #message(found python linux)
    INCLUDEPATH += $$(PYTHON_PATH)
    PYTHON_FOUND = 1
}

!equals( PYTHON_FOUND , 1){
    error (Python directory needs to be configured in environment variable PYTHON_PATH. eg. C:/Python27 )
}


defaults.pri
