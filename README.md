# crystalTestFrameworkApp
Framework that tests hardware and software with Lua scripts.

## Purpose
The Test Framework exists to have software support for testing hardware devices.
Typical situations include:
* Having a circuit board and a multimeter and wanting to measure and write down resistor values
  * The Test Framework reads the value from the multimeter via USB triggered by the press of a button or foot pedal, puts them in a list and at the end displays a report of the measurements
* Having a device that measures something which needs to be monitored
  * The Test Framework reads the values periodically and enters them into a graph to be displayed on screen and saved into a .csv file for further processing
* Having a device with a comport/USB connection that needs to be tested
  * The Test Framework connects to the device and runs a script which sets and gets various values via RPC and puts them in a report which displays if the measured values are within predefined tolerances

## Features
* Support for RPC and SCPI so the Test Framework understands how to control devices
* Relatively easy to write Lua scripts to specify the testing logic and data processing while reusing functionality to detect and identify devices
* Relatively easy to use GUI that allows running test scripts on devices without programming knowledge (once they are written)
* Running multiple tests at a time (as long as they don't use the same devices)

## Installation
### List of Dependencies
* C++17
* Qt 5.12
* git
* LibUSB 1.0
* LimeReport
* Google test and Google mock
* liblua 5.3
* qRPCRuntimeParser

### Windows
1. C++17 See below, MinGW 7.3 includes g++ 7.3 which supports C++17
   1. Compiling with MSVC has not been tested
1. Install Qt via [the installer](https://www.qt.io/download-thank-you)
   1. Open the tree widget of the installer on *Qt 5.12* or later and select the checkbox *MinGW 7.3.0 64-bit* (Do not just select all of Qt 5.12 and install it for all compilers and platforms! It'll take forever.)
   1. Select *Qt Script (Deprecated)* for the same Qt version
   1. Open the *Developer and Designer Tools* branch and select *MinGW 7.3.0 64-bit*
1. Install [git](https://gitforwindows.org/) to the default path `C:\Program Files\Git`
   1. This is necessary for compiling the Test Framework, so you need it even if you obtained the code elsewhere
1. To download the Test Framework open the folder you want to put the Test Framework into, rightclick and select *Git Bash Here* and enter `git clone https://github.com/Crystal-Photonics/crystalTestFrameworkApp --recursive`
1. To build LimeReport open Qt Creator, select *File* -> *Open File or Project...*, navigate to the *crystalTestFrameworkApp*  folder and then to *libs/LimeReport* and select *limereport.pro*.
   1. Should the folder *crystalTestFrameworkApp/libs/LimeReport* be empty you forgot the `--recursive` part of the clone command. Use `git submodule update --init --recursive` in the *crystalTestFrameworkApp* folder to get all the submodules.
   1. Select the Qt version you downloaded in step 2.i. and select *Configure Project*
   1. Press CTRL+B to build
   1. Rightclick on the project *limereport* and select *Close Project "limereport"*
   1. There is no apparent effect and you may feel like nothing happened, but it built the limereport library which the Test Framework relies on
1. To build QWT get the source from (See [readme](libs/qwt/QWT-Readme.txt)) and follow the instructions.
   1. Download and extract the zip file
   1. Open *C:\qwt-6.1.3\qwt.pro*
   1. Follow same steps as for LimeReport to build QWT
   1. Copy *C:\build-qwt-Desktop_Qt_5_12_0_MinGW_64_bit-Debug/libs/\** to *crystalTestFrameworkApp/libs/qwt/lib/5.12.0*
      1. Note that the 5.12.0 refers to the installed Qt version. If you installed Qt 5.13.0 use that instead.
      1. If you are unsure what exactly you need to name the folder containing the Qt version number look at the build path: *build-qwt-Desktop_Qt_**5_12_0**_MinGW_64_bit-Debug* -> *5.12.0*
   1. Copy *C:\qwt-6.1.3/src/\*.h* to  *crystalTestFrameworkApp/libs/qwt/include*
   1. Rightclick on the *qwt* project and select *Close Project "qwt"*
1. To build Google test/moc use Qt Creator to open *crystalTestFrameworkApp/libs/googletest/CMakeLists.txt*
   1. Follow the same procedure, select the toolkit, press CTRL+B, close project *googletest-distribution*
1. To build the Test Framework open *crystalTestFrameworkApp/crystalTestFrameworkApp.pro* in Qt Creator and follow the same steps as for LimeReport
   1. After building is complete press F5 (with debugger) or CTRL+R (without debugger) to start the program.
 
### Linux
TODO
