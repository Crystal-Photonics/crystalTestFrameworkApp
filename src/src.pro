include(../defaults.pri)

TARGET = crystalTestFrameworkApp

TEMPLATE = lib

DEFINES += EXPORT_LIBRARY

SOURCES += mainwindow.cpp \
    pathsettingswindow.cpp \
    config.cpp \
    Protocols/rpcprotocol.cpp \
    console.cpp \
    Protocols/protocol.cpp \
    qt_util.cpp
SOURCES += scriptengine.cpp
SOURCES += util.cpp
SOURCES += CommunicationDevices/communicationdevice.cpp
SOURCES += CommunicationDevices/comportcommunicationdevice.cpp
SOURCES += CommunicationDevices/echocommunicationdevice.cpp
SOURCES += CommunicationDevices/socketcommunicationdevice.cpp

HEADERS += mainwindow.h \
    pathsettingswindow.h \
    config.h \
    Protocols/rpcprotocol.h \
    console.h \
    Protocols/protocol.h \
    qt_util.h
HEADERS += scriptengine.h
HEADERS += export.h
HEADERS += util.h
HEADERS +=CommunicationDevices/communicationdevice.h
HEADERS +=CommunicationDevices/comportcommunicationdevice.h
HEADERS +=CommunicationDevices/echocommunicationdevice.h
HEADERS +=CommunicationDevices/socketcommunicationdevice.h

FORMS    += mainwindow.ui \
    pathsettingswindow.ui
