@echo off
REM Download tinyfiledialogs library

echo ========================================
echo tinyfiledialogs Library Downloader
echo ========================================
echo.

cd /d %~dp0..\third_party

echo Checking if tinyfiledialogs already exists...
if exist "tinyfiledialogs\tinyfiledialogs.h" (
    echo tinyfiledialogs already installed!
    echo.
    goto :end
)

echo Downloading tinyfiledialogs from GitHub...
echo.

REM Create directory
if not exist "tinyfiledialogs" mkdir tinyfiledialogs

REM Download using PowerShell
powershell -Command "& {Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/native-toolkit/tinyfiledialogs/master/tinyfiledialogs.h' -OutFile 'tinyfiledialogs\tinyfiledialogs.h'}"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to download tinyfiledialogs.h!
    goto :error
)

powershell -Command "& {Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/native-toolkit/tinyfiledialogs/master/tinyfiledialogs.c' -OutFile 'tinyfiledialogs\tinyfiledialogs.c'}"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to download tinyfiledialogs.c!
    goto :error
)

powershell -Command "& {Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/native-toolkit/tinyfiledialogs/master/LICENSE' -OutFile 'tinyfiledialogs\LICENSE.txt'}"

echo.
echo ========================================
echo Download completed successfully!
echo ========================================
echo.
echo Files downloaded:
echo   - tinyfiledialogs\tinyfiledialogs.h
echo   - tinyfiledialogs\tinyfiledialogs.c
echo   - tinyfiledialogs\LICENSE.txt
echo.
goto :end

:error
echo.
echo ========================================
echo ERROR: Download failed!
echo ========================================
echo.
echo Please download manually from:
echo https://github.com/native-toolkit/tinyfiledialogs
echo.
exit /b 1

:end
pause
