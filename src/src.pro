include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

SOURCES += mainwindow.cpp \
    scriptengine.cpp \
    pysys.cpp \
    util.cpp \
    main.cpp \
    CommunicationDevices/communicationdevice.cpp \
    CommunicationDevices/comportcommunicationdevice.cpp \
    CommunicationDevices/echocommunicationdevice.cpp \
    CommunicationDevices/socketcommunicationdevice.cpp

HEADERS += mainwindow.h \
    scriptengine.h \
    pysys.h \
    export.h \
    util.h \
    comportcommunicationdevice.h \
    CommunicationDevices/communicationdevice.h \
    CommunicationDevices/comportcommunicationdevice.h \
    CommunicationDevices/echocommunicationdevice.h \
    CommunicationDevices/socketcommunicationdevice.h
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
