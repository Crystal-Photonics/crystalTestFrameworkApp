from: http://pythonqt.sourceforge.net/Building.html

Building

PythonQt requires at least Qt 4.6.1 (for earlier Qt versions, you will need to run the pythonqt_generator, Qt 4.3 is the absolute minimum) and Python 2.6.x/2.7.x or Python 3.3 (or higher). To compile PythonQt, you will need a python developer installation which includes Python's header files and the python2x.[lib | dll | so | dynlib]. The recommended way to build PythonQt is to use the QMake-based *.pro file. The build scripts a currently set to use Python 2.6. You may need to tweak the build/python.prf file to set the correct Python includes and libs on your system.

On Windows, the (non-source) Python Windows installer can be used. Make sure that you use the same compiler as the one that your Python distribution is built with. If you want to use another compiler, you will need to build Python yourself, using your compiler.

To build PythonQt, you need to set the environment variable PYTHON_PATH to point to the root dir of the python installation and PYTHON_LIB to point to the directory where the python lib file is located.

When using the prebuild Python installer, this will be:
> set PYTHON_PATH = c:\Python26
> set PYTHON_LIB = c:\Python26\libs

When using the python sources, this will be something like:
> set PYTHON_PATH = c:\yourDir\Python-2.6.1\
> set PYTHON_LIB = c:\yourDir\Python-2.6.1\PCbuild8\Win32

To build all, do the following (after setting the above variables):
> cd PythonQtRoot
> vcvars32
> qmake
> nmake

This should build everything. If Python can not be linked or include files can not be found, you probably need to tweak build/python.prf

The tests and examples are located in PythonQt/lib.

When using a Python distribution, the debug build typically does not work because the pythonxx_d.lib/.dll are not provided. You can tweak linking of the debug build to the release Python version, but this typically requires patching pyconfig.h and removing Py_DEBUG and linker pragmas (google for it!).

On Linux, you need to install a Python-dev package. If Python can not be linked or include files can not be found, you probably need to tweak build/python.prf

To build PythonQt, just do a:
> cd PythonQtRoot
> qmake
> make all

The tests and examples are located in PythonQt/lib. You should add PythonQt/lib to your LD_LIBRARY_PATH so that the runtime linker can find the *.so files.

On Mac, Python is installed as a Framework, so you should not need to install it. To build PythonQt, just do a:
> cd PythonQtRoot
> qmake
> make all
Tests

There is a unit test that tests most features of PythonQt, see the tests subdirectory for details.
