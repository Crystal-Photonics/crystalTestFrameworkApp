#-------------------------------------------------
#
# Project created by QtCreator 2015-05-07T17:15:19
#
#-------------------------------------------------


TARGET = crystalTestFrameworkApp
TEMPLATE = subdirs

SUBDIRS += app
SUBDIRS += src
SUBDIRS += tests
SUBDIRS += libs/QtRPT

#src.depends = comModules/mocklayer/appPlugin
app.depends = src
tests.depends = src
src.depends = libs/QtRPT
