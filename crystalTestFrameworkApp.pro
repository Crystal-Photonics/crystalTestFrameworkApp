#-------------------------------------------------
#
# Project created by QtCreator 2015-05-07T17:15:19
#
#-------------------------------------------------


TARGET = crystalTestFrameworkApp
TEMPLATE = subdirs

SUBDIRS = src
SUBDIRS += app
SUBDIRS += tests

#comModules/mocklayer/appPlugin

#src.depends = comModules/mocklayer
app.depends = src
tests.depends = src
