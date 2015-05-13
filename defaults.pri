

QT = gui core

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SRC_DIR = $$PWD

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/libs/PythonQt3.0/src

LIBS += -L$$PWD/libs/PythonQt3.0/lib -lPythonQt  -lPythonQt_QtAll
