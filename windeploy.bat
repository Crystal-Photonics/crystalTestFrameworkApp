PATH_old=%PATH%
PATH=%PATH%;C:\Qt\Tools\mingw530_32\bin

echo %PATH%

set CTFW_RELEASE_PATH=..\build-crystalTestFrameworkApp-Desktop_Qt_5_8_0_MinGW_32bit-Release\bin\release\
set QWT_PATH=C:\Qt\build-qwt-Desktop_Qt_5_8_0_MinGW_32bit-Release\lib\

copy libs\LimeReport\build\5.8.0\win32\release\lib\limereport.dll %CTFW_RELEASE_PATH%
copy libs\LimeReport\build\5.8.0\win32\release\lib\QtZint.dll %CTFW_RELEASE_PATH%

copy %QWT_PATH%qwt.dll %CTFW_RELEASE_PATH%

C:\Qt\5.8\mingw53_32\bin\windeployqt -network -serialport -sql -opengl -printsupport -script -xml -test %CTFW_RELEASE_PATH%crystalTestFramework.exe 

set PATH=%PATH_old%

