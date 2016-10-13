include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

SOURCES += mainwindow.cpp
SOURCES += scriptengine.cpp
SOURCES += pysys.cpp
SOURCES += util.cpp
SOURCES += main.cpp
SOURCES += CommunicationDevices/communicationdevice.cpp
SOURCES += CommunicationDevices/comportcommunicationdevice.cpp
SOURCES += CommunicationDevices/echocommunicationdevice.cpp
SOURCES += CommunicationDevices/socketcommunicationdevice.cpp

HEADERS += mainwindow.h
HEADERS += scriptengine.h
HEADERS += pysys.h
HEADERS += export.h
HEADERS += util.h
HEADERS +=comportcommunicationdevice.h
HEADERS +=CommunicationDevices/communicationdevice.h
HEADERS +=CommunicationDevices/comportcommunicationdevice.h
HEADERS +=CommunicationDevices/echocommunicationdevice.h
HEADERS +=CommunicationDevices/socketcommunicationdevice.h
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


