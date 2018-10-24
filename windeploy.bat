SET PATH=%PATH%;C:\Qt\Tools\mingw530_32\bin

echo %PATH%

set CTFW_RELEASE_PATH_local=..\build-crystalTestFrameworkApp-Desktop_Qt_5_8_0_MinGW_32bit-Release\bin\release
set CTFW_RELEASE_PATH_remote=\\amelie\austausch\Alle\CrystalTestFramework\_executables\test_framework

set QWT_PATH=libs\qwt

copy libs\LimeReport\build\5.8.0\win32\release\lib\limereport.dll %CTFW_RELEASE_PATH_local%\ || goto :FAIL
copy libs\LimeReport\build\5.8.0\win32\release\lib\QtZint.dll %CTFW_RELEASE_PATH_local%\ || goto :FAIL

copy %QWT_PATH%\lib\5.8.0\qwt.dll %CTFW_RELEASE_PATH_local%\ || GOTO :FAIL

C:\Qt\5.8\mingw53_32\bin\windeployqt -network -serialport -sql -opengl -printsupport -script -xml -test %CTFW_RELEASE_PATH_local%\crystalTestFramework.exe  || GOTO :FAIL

xco_py /s /Y %CTFW_RELEASE_PATH_local% %CTFW_RELEASE_PATH_remote%

@goto :EOF
:FAIL
@echo off
IF %ERRORLEVEL% NEQ 0 (
	echo Error %ERRORLEVEL%, aborting
	EXIT /B %ERRORLEVEL%
)

