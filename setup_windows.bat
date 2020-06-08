set QMAKE="\Qt\5.13.0\mingw73_64\bin\qmake"
set MAKE="\Qt\Tools\mingw730_64\bin\mingw32-make"
set CMAKE="cmake"

@rem Testing if qmake, make and cmake work

@%QMAKE% --version || echo Failed finding qmake. Please install Qt 5.13+ and set the path to its qmake in setup_windows.bat. && exit 1
@%MAKE% --version || echo Failed finding make. Please install Qt's Mingw 7.3+ and set the path to mingw32-make in setup_windows.bat. && exit 1
@%CMAKE% --version || echo Failed finding cmake. Please install it from https://cmake.org/download/ and set the path to it in setup_windows.bat. && exit 1

@rem initializing the submodules
git submodule update --init --recursive || exit 1

@rem build limereport
cd libs
mkdir build_limereport
cd build_limereport
%QMAKE% ../LimeReport/limereport.pro -spec win32-g++ "CONFIG+=debug" || exit 1
%MAKE% -j%NUMBER_OF_PROCESSORS% qmake_all || exit 1
%MAKE% -j%NUMBER_OF_PROCESSORS% || exit 1
cd ../..

@rem extract qwt
powershell -command "Expand-Archive -Force '%~dp0libs/qwt/qwt-6.1.3.zip' '%~dp0libs/qwt'" || exit 1
cd libs/qwt
mkdir build_qwt
cd build_qwt

@rem build qwt
%QMAKE% ../qwt-6.1.3/qwt.pro -spec win32-g++ "CONFIG+=debug" || exit 1
%MAKE% -j%NUMBER_OF_PROCESSORS% || exit 1

@rem build googletest and googlemoc
cd ../../googletest
mkdir build
cd build
%CMAKE% -G "Unix Makefiles" .. || exit 1
%MAKE% -j%NUMBER_OF_PROCESSORS% || exit 1
