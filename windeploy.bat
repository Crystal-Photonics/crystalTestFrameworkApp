SET PATH=%PATH%;C:\Qt\Tools\mingw530_32\bin

echo %PATH%

set CTFW_RELEASE_PATH=..\build-crystalTestFrameworkApp-Desktop_Qt_5_8_0_MinGW_32bit-Release\bin\release\
set QWT_PATH=libs\qwt

copy libs\LimeReport\build\5.8.0\win32\release\lib\limereport.dll %CTFW_RELEASE_PATH% || goto :FAIL
copy libs\LimeReport\build\5.8.0\win32\release\lib\QtZint.dll %CTFW_RELEASE_PATH% || goto :FAIL

copy %QWT_PATH%\lib\qwt.dll %CTFW_RELEASE_PATH% || GOTO :FAIL

C:\Qt\5.8\mingw53_32\bin\windeployqt -network -serialport -sql -opengl -printsupport -script -xml -test %CTFW_RELEASE_PATH%crystalTestFramework.exe  || GOTO :FAIL

@goto :EOF
:FAIL
@echo off
IF %ERRORLEVEL% NEQ 0 (
	echo Error %ERRORLEVEL%, aborting
	EXIT /B %ERRORLEVEL%
)

