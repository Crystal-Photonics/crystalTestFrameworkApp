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
* Qt 5.13 (slightly older or newer versions should be ok, but require setting the version in the build script)
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
   1. Open the tree widget of the installer on *Qt 5.13* or later and select the checkbox *MinGW 7.3.0 64-bit* (Do not just select all of Qt 5.13 and install it for all compilers and platforms! It will take a long time and use up over 30GB of space unnecessarily.)
   1. Select *Qt Script (Deprecated)* for the same Qt version
   1. Open the *Developer and Designer Tools* branch and select *MinGW 7.3.0 64-bit*
1. Install [git](https://gitforwindows.org/) to the default path `C:\Program Files\Git`
   1. This is necessary for compiling the Test Framework, so you need it even if you obtained the code elsewhere
1. To download the Test Framework open the folder you want to put the Test Framework into, rightclick and select *Git Bash Here* and enter `git clone https://github.com/Crystal-Photonics/crystalTestFrameworkApp`
1. To get and build dependencies double-click `winsetup.bat`. This will take a couple of minutes.
   1. If everything builds successfully the cmd window will close. If an error happens it will stay open.
   1. If you use a QT version that is not 5.13.0, open [setup_windows.bat](setup_windows.bat) and edit the top line to the version you are using. 
1. After all dependencies have been built open Qt Creator, select *File* -> *Open File or Project...*, navigate to the *crystalTestFrameworkApp*  folder and select *crystalTestFrameworkApp.pro*.
   1. Press F5 (with debugger) or CTRL+R (without debugger) to build and start the program.
 
### Linux
TODO
