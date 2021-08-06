#-------------------------------------------------
#
# Project created by QtCreator 2015-05-07T17:15:19
#
#-------------------------------------------------


TARGET = crystalTestFrameworkApp
TEMPLATE = subdirs


SUBDIRS += src
#SUBDIRS += tests
TRAVIS = $$(TRAVIS)
equals(TRAVIS, true){
        #SUBDIRS += tests
} else{
	SUBDIRS += app
}


#src.depends = comModules/mocklayer/appPlugin
app.depends = src
#tests.depends = src

message($$QMAKESPEC)

OTHER_FILES += .clang-format
OTHER_FILES += .travis.yml
OTHER_FILES += lsan.supp
OTHER_FILES += examples/settings/communication_settings.json
OTHER_FILES += examples/settings/environment_variables.json
OTHER_FILES += examples/settings/equipment_data_base.json
OTHER_FILES += examples/settings/exceptional_approvals.json
OTHER_FILES += examples/settings/isotope_sources.json
OTHER_FILES += README.md
OTHER_FILES += doxygen.conf
