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
Note that building the dependencies as well as the Crystal Test Framework itself requires about 10GB of RAM if you use parallel jobs. A computer with 16+GB of RAM is recommended. 8GB works too if you reduce the number of parallel jobs to 3 or so at the cost of increased build times. For the dependency build the `CORES` variable is set to however many cores your computer has in line 3 of [linuxsetup.sh](linuxsetup.sh) or [setup_windows.bat](setup_windows.bat). You can set that variable to a lower number should the build run out of memory.
For the build of the Crystal Test Framework itself go to Qt Creator, on the left side `Projects`, `Build`, open the `Make` details and set the number of parallel jobs accordingly.

### List of Dependencies
* C++17
* Qt 5.13 (slightly older or newer versions should be ok, but require setting the version in the build script)
  * Qt Script
  * Qt Serialport
  * Qt SVG
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
1. After all dependencies have been built open Qt Creator, select *File* -> *Open File or Project...*, navigate to the *crystalTestFrameworkApp* folder and select *crystalTestFrameworkApp.pro*.
   1. Press F5 (with debugger) or CTRL+R (without debugger) to build and start the program.
 
### Linux (tested with Ubuntu 20.04)
1. To obtain the required packages run `sudo apt install g++-8 cmake qtbase5-dev git qtcreator libqwt-qt5-dev libqt5serialport5-dev libqt5svg5-dev qtscript5-dev libusb-1.0.0-0-dev`
   1. If you use another Linux distribution you may need to swap out `apt` for your system's package manager (`pacman`, `yum`, `rpm`, ...).
   1. The package names may differ slightly between package managers and versions. Adapt them as needed.
1. To download the Test Framework open the folder you want to put the Test Framework into and enter `git clone https://github.com/Crystal-Photonics/crystalTestFrameworkApp`
1. Run [./setup_linux.sh](setup_linux.sh)
   1. The script will compile dependencies. Should it succeed it ends with printing "Setup complete", otherwise the last lines should be an error description.
1. After all dependencies have been built open Qt Creator, select *File* -> *Open File or Project...*, navigate to the *crystalTestFrameworkApp* folder and select *crystalTestFrameworkApp.pro*.
   1. Press F5 (with debugger) or CTRL+R (without debugger) to build and start the program.

Should you use Ubuntu Xenial 16.04 you can look at the [Travis CI build script](.travis.yml) for the commands and the [Travis CI test run](https://travis-ci.org/github/Crystal-Photonics/crystalTestFrameworkApp) for examples of a successful compilation (check previous builds should the current build have failed).
