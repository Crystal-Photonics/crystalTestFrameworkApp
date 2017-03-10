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

#src.depends = comModules/mocklayer/appPlugin
app.depends = src
tests.depends = src

