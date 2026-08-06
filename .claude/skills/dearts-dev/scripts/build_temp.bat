@echo off
call "D:\Programs\Microsoft\Vs2022\VC\Auxiliary\Build\vcvars64.bat"
cd /d D:\develop\CPlusPlus\Dear_SDL\DearTsd\build
cmake --build . --target demo_imhex_style --config Debug
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
) else (
    echo Build failed with error code %ERRORLEVEL%
)
pause
