echo %PATH%
JLink.exe -device STM32L151RE  -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink

