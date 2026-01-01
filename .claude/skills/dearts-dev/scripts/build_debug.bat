@echo off
call "D:\Programs\Microsoft\Vs2022\VC\Auxiliary\Build\vcvars64.bat"
cd /d D:\develop\CPlusPlus\Dear_SDL\DearTsd\build
cmake --build . --config Debug
pause
