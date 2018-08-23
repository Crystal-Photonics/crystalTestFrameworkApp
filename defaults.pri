CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QT = gui core network serialport xml svg
QT += script sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += $$PWD/src
INCLUDEPATH += ../libs/luasol/include


win32 {
    QWT_DIR = $$PWD/libs/qwt
    INCLUDEPATH += $$QWT_DIR/include
    #INCLUDEPATH += $$QWT_DIR/include/qwt
    LIBS += -L$$QWT_DIR/lib

    CONFIG(debug, debug|release) {
        LIBS += -lqwtd
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/win32/debug/lib
    } else {
        LIBS += -lqwt
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/win32/release/lib
    }

    LIBS += -L$$PWD/libs/luasol
    LIBS += -llua53
    SH = C:/Program Files/Git/bin/sh.exe

}else{
    CONFIG += qwt
    LIBS += -lqwt
    LIBS += -llua5.3
   # LIBS += -L/usr/local/qwt-svn/lib
   # LIBS += -L/usr/local/qwt-svn/lib

    #error("fill in the correct path for linux")
    CONFIG(debug, debug|release) {
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux64/debug/lib
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux/debug/lib
    } else {
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux64/release/lib
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux/release/lib
    }

    #message($$LIBS)
    SH = sh

}

win32 {
    INCLUDEPATH += $$PWD/libs/libusb-1.0.21/include/
    #message($$INCLUDEPATH)
    LIBS += -L$$PWD/libs/libusb-1.0.21/MinGW32/static/
    LIBS += -llibusb-1.0
}else{
    INCLUDEPATH += $$PWD/libs/libusb-1.0.21/include/
    LIBS +=  -lusb-1.0
}




INCLUDEPATH += $$PWD/libs/LimeReport/include

CONFIG(debug, debug|release) {
    LIBS += -llimereportd
    LIBS += -lQtZintd

    BINSUB_DIR = bin/debug
    BINDIR = $$OUT_PWD/../$$BINSUB_DIR
    BINDIR_ZINT = $$OUT_PWD/../../../$$BINSUB_DIR
} else {
    LIBS += -llimereport
    LIBS += -lQtZint

    BINSUB_DIR = bin/release
    BINDIR = $$OUT_PWD/../$$BINSUB_DIR
    BINDIR_ZINT = $$OUT_PWD/../../../$$BINSUB_DIR
}


DEFINES += SOL_CHECK_ARGUMENTS

QMAKE_CXXFLAGS += -Werror

QMAKE_CXXFLAGS_DEBUG += -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
unix {
	QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
} else {
	QMAKE_CXXFLAGS_DEBUG += -fsanitize-undefined-trap-on-error
}
QMAKE_CXXFLAGS_RELEASE += -Wall -Wunused-function -Wunused-parameter -Wunused-variable

QMAKE_CXXFLAGS_DEBUG += -g -fno-omit-frame-pointer
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...


