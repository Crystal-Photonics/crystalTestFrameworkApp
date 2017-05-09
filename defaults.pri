CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QT = gui core network serialport xml
QT += script sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += $$PWD/src
INCLUDEPATH += ../libs/luasol/include
INCLUDEPATH += ../libs/QtRPT/include

win32 {
    QWT_DIR = $$PWD/libs/qwt
    INCLUDEPATH += $$QWT_DIR/include
    LIBS += -L$$QWT_DIR/lib
    Debug:LIBS += -lqwtd
    Release:LIBS += -lqwt

    LIBS += -L$$PWD/libs/luasol
    LIBS += -llua53
}else{
    CONFIG += qwt
    LIBS += -llua5.3
}

win32 {
    INCLUDEPATH += $$PWD/libs/libusb-1.0.21/include/
    #message($$INCLUDEPATH)
    LIBS += -L$$PWD/libs/libusb-1.0.21/MinGW32/static/
    LIBS += -llibusb-1.0
}else{
    LIBS += -llibusb-1.0
}

DEFINES += SOL_CHECK_ARGUMENTS

QMAKE_CXXFLAGS += -Werror

QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
QMAKE_CXXFLAGS_RELEASE += -Wunused-function -Wunused-parameter -Wunused-variable

QMAKE_CXXFLAGS_DEBUG += -g -fno-omit-frame-pointer
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...

CONFIG(debug, debug|release) {
    BINSUB_DIR = bin/debug
    BINDIR = $$OUT_PWD/../$$BINSUB_DIR
	BINDIR_ZINT = $$OUT_PWD/../../../$$BINSUB_DIR
} else {
    BINSUB_DIR = bin/release
    BINDIR = $$OUT_PWD/../$$BINSUB_DIR
	BINDIR_ZINT = $$OUT_PWD/../../../$$BINSUB_DIR
}
