CONFIG += c++17
CONFIG += strict_c++
QMAKE_CXXFLAGS += -std=c++17

QT = gui core network serialport xml svg
QT += script sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

GCC_MACHINE = $$system("g++ -dumpmachine")
message($$GCC_MACHINE)

INCLUDEPATH += $$PWD/src
QMAKE_CXXFLAGS += -isystem $$PWD/libs/luasol/include

win32 {
	QWT_DIR = $$PWD/libs/qwt
	QMAKE_CXXFLAGS += -isystem $$QWT_DIR/qwt-6.1.3/src
	LIBS += -L$$QWT_DIR/build_qwt/lib

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

	TRAVIS = $$(TRAVIS)
	equals(TRAVIS, true){
		LIBS += -lqwt
	} else{
		LIBS += -lqwt-qt5
	}

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
	QMAKE_CXXFLAGS += -isystem $$PWD/libs/libusb-1.0.21/include/
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
	QMAKE_CXXFLAGS += -isystem $$PWD/libs/libusb-1.0.21/include/
	LIBS +=  -lusb-1.0
}




QMAKE_CXXFLAGS += -isystem $$PWD/libs/LimeReport/include

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

QMAKE_CXXFLAGS += -Werror -ftemplate-depth=1000
#QMAKE_CXXFLAGS += -Werror

QMAKE_CXXFLAGS_DEBUG += -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
QMAKE_CXXFLAGS_RELEASE += -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare
#-Wno-error=noexcept-type unsupported

unix {
    equals(QMAKE_CXX, g++) {
        QMAKE_CXXFLAGS_DEBUG += -fno-var-tracking-assignments -fno-merge-debug-strings
        #QMAKE_CXXFLAGS_DEBUG += -Wno-deprecated-copy #doesn't work on Travis
    }
    eval("SANITIZERS = $$(SANITIZER)")
    message("Using sanitizer $$SANITIZERS")
    equals(SANITIZERS, "") {
        QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
        QMAKE_LFLAGS_DEBUG += -fsanitize=undefined,address
    } else {
        QMAKE_CXXFLAGS_DEBUG += -fsanitize=$$SANITIZERS
        QMAKE_LFLAGS_DEBUG += -fsanitize=$$SANITIZERS
    }
    TRAVIS = $$(TRAVIS)
    equals(TRAVIS, true){
        QMAKE_LFLAGS_DEBUG += -fuse-ld=gold -L/usr/local/lib
    }
} else {
    QMAKE_CXXFLAGS_DEBUG += -fsanitize-undefined-trap-on-error
    QMAKE_CXXFLAGS_DEBUG +=  -Wa,-mbig-obj
}
QMAKE_CXXFLAGS_RELEASE += -Wall -Wunused-function -Wunused-parameter -Wunused-variable -O2
QMAKE_CXXFLAGS_DEBUG += -ggdb -fno-omit-frame-pointer -Og
QMAKE_CXXFLAGS += -fdiagnostics-color=always
#QMAKE_CXXFLAGS_DEBUG += -fsanitize=undefined,address
#QMAKE_CXXFLAGS_DEBUG += -static-libasan -static-libubsan #some day windows will support a reasonable development environment ...


