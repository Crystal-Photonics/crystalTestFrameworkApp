CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QT = gui core network serialport xml
QT += script sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += $$PWD/src
INCLUDEPATH += ../libs/luasol/include


win32 {
    QWT_DIR = $$PWD/libs/qwt
    INCLUDEPATH += $$QWT_DIR/include
    LIBS += -L$$QWT_DIR/lib
    debug:LIBS += -lqwtd
    release:LIBS += -lqwt

    LIBS += -L$$PWD/libs/luasol
    LIBS += -llua53

        debug:LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/win32/debug/lib
        release:LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/win32/release/lib

}else{
    CONFIG += qwt
    LIBS += -llua5.3
        #error("fill in the correct path for linux")
        debug:LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux64/debug/lib
        debug:LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux/debug/lib
        release:LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux64/release/lib
        release:LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/linux/release/lib
        message($$LIBS)
#libs/LimeReport/build/5.5.1/linux64/debug/lib/
}

win32 {
    INCLUDEPATH += $$PWD/libs/libusb-1.0.21/include/
    #message($$INCLUDEPATH)
    LIBS += -L$$PWD/libs/libusb-1.0.21/MinGW32/static/
    LIBS += -llibusb-1.0
}else{
    LIBS +=  -lusb-1.0
}

INCLUDEPATH += $$PWD/libs/LimeReport/include
LIBS += -llimereport -lQtZint
DEFINES += SOL_CHECK_ARGUMENTS

QMAKE_CXXFLAGS += -Werror

QMAKE_CXXFLAGS_DEBUG += -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
QMAKE_CXXFLAGS_RELEASE += -Wall -Wunused-function -Wunused-parameter -Wunused-variable

QMAKE_CXXFLAGS_DEBUG += -g -fno-omit-frame-pointer
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...

CONFIG(debug, debug|release) {
    BINSUB_DIR = bin/debug
    BINDIR = $$OUT_PWD/../$$BINSUB_DIR#
   BINDIR_ZINT = $$OUT_PWD/../../../$$BINSUB_DIR
} else {
    BINSUB_DIR = bin/release
    BINDIR = $$OUT_PWD/../$$BINSUB_DIR
        BINDIR_ZINT = $$OUT_PWD/../../../$$BINSUB_DIR
}
