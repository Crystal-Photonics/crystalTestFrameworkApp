CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

QT = gui core network serialport xml svg
QT += script sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

GCC_MACHINE = $$system("g++ -dumpmachine")
message($$GCC_MACHINE)

INCLUDEPATH += $$PWD/src
INCLUDEPATH += ../libs/luasol/include


win32 {
    QWT_DIR = $$PWD/libs/qwt/
    INCLUDEPATH += $$QWT_DIR/include
    LIBS += -L$$QWT_DIR/lib/$${QT_VERSION}

    CONFIG(debug, debug|release) {
        LIBS += -lqwtd
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/win32/debug/lib
    } else {
        LIBS += -lqwt
        LIBS += -L$$PWD/libs/LimeReport/build/$${QT_VERSION}/win32/release/lib
    }
    equals(GCC_MACHINE,  x86_64-w64-mingw32){
        LIBS += -L$$PWD/libs/luasol/win64
        message(Win32 64bit)
    }
    equals(GCC_MACHINE, i686-w64-mingw32){
        LIBS += -L$$PWD/libs/luasol/win32
        message(Win32 32bit)
    }
    LIBS += -llua53
    SH = C:/Program Files/Git/bin/sh.exe

}else{
    CONFIG += qwt
    LIBS += -lqwt
    LIBS += -llua5.3

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
    equals(GCC_MACHINE,  x86_64-w64-mingw32){
        LIBS += -L$$PWD/libs/libusb-1.0.21/MinGW64/static/
      #  message(Win32 64bit)
    }
    equals(GCC_MACHINE, i686-w64-mingw32){
        LIBS += -L$$PWD/libs/libusb-1.0.21/MinGW32/static/
       # message(Win32 32bit)
    }

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
	eval("SANITIZERS = $$(SANITIZER)")
	message("Using sanitizer $$SANITIZERS")
	equals(SANITIZERS, "") {
		QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
		QMAKE_LFLAGS_DEBUG += -fsanitize=undefined,address
	} else {
		QMAKE_CXXFLAGS_DEBUG += -fsanitize=$$SANITIZERS
		QMAKE_LFLAGS_DEBUG += -fsanitize=$$SANITIZERS
	}
	QMAKE_LFLAGS_DEBUG += -fuse-ld=gold -L/usr/local/lib
} else {
	QMAKE_CXXFLAGS_DEBUG += -fsanitize-undefined-trap-on-error
}
QMAKE_CXXFLAGS_RELEASE += -Wall -Wunused-function -Wunused-parameter -Wunused-variable -O1
QMAKE_CXXFLAGS_DEBUG += -g -fno-omit-frame-pointer -O1
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...


