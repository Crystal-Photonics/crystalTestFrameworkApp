QT += gui xml script sql printsupport

SUBDIRS += $$PWD/zint/backend_qt4

TARGET = QtRPT
TEMPLATE = lib

CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14

INCLUDEPATH += ../include
INCLUDEPATH += ../zint/backend
INCLUDEPATH += ../zint/backend_qt4

LIBS += -L../zint
LIBS += -lQtZint

HEADERS += \
	../include/Barcode.h \
	../include/chart.h \
	../include/CommonClasses.h \
	../include/qtrpt.h \
	../include/qtrpt_global.h \
	../include/qtrptnamespace.h \
	../include/RptBandObject.h \
	../include/RptCrossTabObject.h \
	../include/RptFieldObject.h \
	../include/RptPageObject.h \
	../include/RptSql.h \
	../include/RptSqlConnection.h

SOURCES += \
	../src/Barcode.cpp \
	../src/chart.cpp \
	../src/CommonClasses.cpp \
	../src/qtrpt.cpp \
	../src/RptBandObject.cpp \
	../src/RptCrossTabObject.cpp \
	../src/RptFieldObject.cpp \
	../src/RptPageObject.cpp \
	../src/RptSql.cpp \
	../src/RptSqlConnection.cpp
