how to build with MinGW
set you python version in file build/python.prf

searchfor *.lib references and change them to lib*.a
	in build\PythonQt_QtAll.prf:
	change to:
		#win32::LIBS += $$PWD/../lib/PythonQt_QtAll$${DEBUG_EXT}.lib
		#unix::
		LIBS += -L$$PWD/../lib -lPythonQt_QtAll$${DEBUG_EXT}
		
		
	in build\PythonQt.prf:
	change to
		#win32::LIBS += $$PWD/../lib/PythonQt$${DEBUG_EXT}.lib
		#unix::
		LIBS += -L$$PWD/../lib -lPythonQt$${DEBUG_EXT}
		
	in build/python.prf:
	change to
		win32:LIBS += $$(PYTHON_LIB)/libpython$${PYTHON_VERSION}$${DEBUG_EXT}.a
		
	in src\src.pri:
	change to:
		#win32:QMAKE_CXXFLAGS += /bigobj