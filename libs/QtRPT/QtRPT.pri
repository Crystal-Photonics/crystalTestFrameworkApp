include(../../defaults.pri)
DESTDIR = $$BINDIR

QT += gui xml script sql printsupport

CONFIG += c++14
CONFIG += warn_off
QMAKE_CXXFLAGS += -std=c++14

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/zint/backend
INCLUDEPATH += $$PWD/zint/backend_qt4

LIBS += -L$$BINDIR
LIBS += -lQtZint

HEADERS += \
	$$PWD/include/Barcode.h \
	$$PWD/include/chart.h \
	$$PWD/include/CommonClasses.h \
	$$PWD/include/qtrpt.h \
	$$PWD/include/qtrpt_global.h \
	$$PWD/include/qtrptnamespace.h \
	$$PWD/include/RptBandObject.h \
	$$PWD/include/RptCrossTabObject.h \
	$$PWD/include/RptFieldObject.h \
	$$PWD/include/RptPageObject.h \
	$$PWD/include/RptSql.h \
	$$PWD/include/RptSqlConnection.h

SOURCES += \
	$$PWD/src/Barcode.cpp \
	$$PWD/src/chart.cpp \
	$$PWD/src/CommonClasses.cpp \
	$$PWD/src/qtrpt.cpp \
	$$PWD/src/RptBandObject.cpp \
	$$PWD/src/RptCrossTabObject.cpp \
	$$PWD/src/RptFieldObject.cpp \
	$$PWD/src/RptPageObject.cpp \
	$$PWD/src/RptSql.cpp \
	$$PWD/src/RptSqlConnection.cpp
