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

#src.depends = comModules/mocklayer/appPlugin
app.depends = src
tests.depends = src

OTHER_FILES += .travis.yml
OTHER_FILES += lsan.supp
OTHER_FILES += examples/settings/communication_settings.json
OTHER_FILES += examples/settings/environment_variables.json
OTHER_FILES += examples/settings/equipment_data_base.json
OTHER_FILES += examples/settings/exceptional_approvals.json
OTHER_FILES += examples/settings/isotope_sources.json
