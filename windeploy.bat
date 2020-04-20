SET PATH=C:\Qt\Tools\mingw730_64\bin;%PATH%

SET QT_VERSION=5.12.0
SET MINGW_VERSION=Qt_5_12_0_MinGW_64bit

echo %PATH%

set CTFW_RELEASE_PATH_local=..\build-crystalTestFrameworkApp-Desktop_%MINGW_VERSION%-Release\bin\release
set CTFW_RELEASE_PATH_remote=\\amelie\austausch\Alle\CrystalTestFramework\_executables\test_framework

set QWT_PATH=libs\qwt

copy libs\LimeReport\build\%QT_VERSION%\win32\release\lib\limereport.dll %CTFW_RELEASE_PATH_local%\ || goto :FAIL
copy libs\LimeReport\build\%QT_VERSION%\win32\release\lib\QtZint.dll %CTFW_RELEASE_PATH_local%\ || goto :FAIL

copy %QWT_PATH%\lib\%QT_VERSION%\qwt.dll %CTFW_RELEASE_PATH_local%\ || GOTO :FAIL

C:\Qt\%QT_VERSION%\mingw73_64\bin\windeployqt -network -serialport -sql -opengl -printsupport -script -xml -test %CTFW_RELEASE_PATH_local%\crystalTestFramework.exe  || GOTO :FAIL

xcopy /s /Y %CTFW_RELEASE_PATH_local% %CTFW_RELEASE_PATH_remote%

@goto :EOF
:FAIL
@echo off
IF %ERRORLEVEL% NEQ 0 (
	echo Error %ERRORLEVEL%, aborting
	EXIT /B %ERRORLEVEL%
)

