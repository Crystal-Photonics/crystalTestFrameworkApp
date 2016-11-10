include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

SOURCES += mainwindow.cpp
SOURCES += scriptengine.cpp
SOURCES += util.cpp
SOURCES += main.cpp
SOURCES += CommunicationDevices/communicationdevice.cpp
SOURCES += CommunicationDevices/comportcommunicationdevice.cpp
SOURCES += CommunicationDevices/echocommunicationdevice.cpp
SOURCES += CommunicationDevices/socketcommunicationdevice.cpp

HEADERS += mainwindow.h
HEADERS += scriptengine.h
HEADERS += export.h
HEADERS += util.h
HEADERS +=CommunicationDevices/communicationdevice.h
HEADERS +=CommunicationDevices/comportcommunicationdevice.h
HEADERS +=CommunicationDevices/echocommunicationdevice.h
HEADERS +=CommunicationDevices/socketcommunicationdevice.h
HEADERS += commodulinterface.h


FORMS    += mainwindow.ui
